#![allow(dead_code)]

use bootloader_api::info::{FrameBuffer, PixelFormat};
use core::fmt::{self, Write};
use lazy_static::lazy_static;
use spin::Mutex;

/* ---------- Color + writer ---------- */

#[derive(Copy, Clone)]
pub struct Rgb {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

pub struct TextWriter {
    fb: *mut FrameBuffer, // raw pointer so we can stash in a static
    fg: Rgb,
    bg: Option<Rgb>, // None = transparent
    cx: u32,
    cy: u32, // cursor (cell coords)
    cell_w: u32,
    cell_h: u32, // glyph scale (8x8 base)
    init: bool,
}

// SAFETY: TextWriter is only used in a single-threaded context or protected by a Mutex.
unsafe impl Send for TextWriter {}
unsafe impl Sync for TextWriter {}

impl TextWriter {
    const fn uninit() -> Self {
        Self {
            fb: core::ptr::null_mut(),
            fg: Rgb {
                r: 255,
                g: 255,
                b: 255,
            },
            bg: Some(Rgb { r: 0, g: 0, b: 0 }),
            cx: 0,
            cy: 0,
            cell_w: 8,
            cell_h: 8,
            init: false,
        }
    }
    pub fn init(&mut self, fb: &'static mut FrameBuffer) {
        self.fb = fb as *mut _;
        self.init = true;
        self.clear(Rgb { r: 0, g: 0, b: 0 });
    }
    #[inline]
    fn is_init(&self) -> bool {
        self.init && !self.fb.is_null()
    }
    #[inline]
    fn fb_mut(&mut self) -> &mut FrameBuffer {
        unsafe { &mut *self.fb }
    }

    pub fn set_colors(&mut self, fg: Rgb, bg: Option<Rgb>) {
        self.fg = fg;
        self.bg = bg;
    }
    pub fn set_cursor(&mut self, col: u32, row: u32) {
        self.cx = col;
        self.cy = row;
    }
    pub fn set_cell_size(&mut self, w: u32, h: u32) {
        self.cell_w = w;
        self.cell_h = h;
    }

    pub fn write_str_raw(&mut self, s: &str) {
        for b in s.bytes() {
            match b {
                b'\n' => self.newline(),
                b'\r' => self.cx = 0,
                _ => self.put_char(b as char),
            }
        }
    }
    fn newline(&mut self) {
        self.cx = 0;
        self.cy += 1;
        let rows = self.rows();
        if self.cy >= rows {
            self.scroll_up(1);
            self.cy = rows - 1;
        }
    }
    fn cols(&mut self) -> u32 {
        (self.fb_mut().info().width / self.cell_w as usize) as u32
    }
    fn rows(&mut self) -> u32 {
        (self.fb_mut().info().height / self.cell_h as usize) as u32
    }

    pub fn put_char(&mut self, ch: char) {
        if !self.is_init() {
            return;
        }
        if self.cx >= self.cols() {
            self.newline();
        }
        let x_px = self.cx * self.cell_w;
        let y_px = self.cy * self.cell_h;
        let cell_w = self.cell_w;
        let cell_h = self.cell_h;
        let fg = self.fg;
        let bg = self.bg;
        draw_glyph(self.fb_mut(), ch, x_px, y_px, cell_w, cell_h, fg, bg);
        self.cx += 1;
    }

    pub fn clear(&mut self, c: Rgb) {
        if !self.is_init() {
            return;
        }
        let info = self.fb_mut().info();
        fill_rect(
            self.fb_mut(),
            0,
            0,
            info.width as u32,
            info.height as u32,
            c,
        );
        self.cx = 0;
        self.cy = 0;
    }

    fn scroll_up(&mut self, lines: u32) {
        let cell_h = self.cell_h;
        let bg = self.bg.unwrap_or(Rgb { r: 0, g: 0, b: 0 }); // Store bg before mutable borrow
        let fb = self.fb_mut();
        let info = fb.info();
        let dy = (lines * cell_h).min(info.height as u32);
        if dy == 0 {
            return;
        }
        let stride = info.stride;
        let bpp = info.bytes_per_pixel;
        let row_bytes = (stride * bpp) as usize;

        let buf = fb.buffer_mut();
        let total = (info.height * row_bytes) as usize;
        let src = (dy * row_bytes as u32) as usize;

        buf.copy_within(src..total, 0);

        // clear bottom
        for y in info.height - dy as usize..info.height {
            for x in 0..info.width {
                put_pixel(fb, x as u32, y as u32, bg);
            }
        }
    }
}

/* implement core::fmt::Write so we can use write!/format_args! */
impl Write for TextWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        if !self.is_init() {
            return Ok(());
        }
        self.write_str_raw(s);
        Ok(())
    }
}

/* ---------- Global TW via lazy_static ---------- */

lazy_static! {
    pub static ref TW: Mutex<TextWriter> = Mutex::new(TextWriter::uninit());
}

/// Call this once in kernel_main with the framebuffer the bootloader gives you.
pub fn init_global(fb: &'static mut FrameBuffer) {
    TW.lock().init(fb);
}

/* ---------- Printing glue + macros ---------- */

pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    let mut tw = TW.lock();
    let _ = tw.write_fmt(args);
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ({
        $crate::text::_print(core::format_args!($($arg)*));
    })
}

#[macro_export]
macro_rules! println {
    () => ({
        $crate::tty::_print(core::format_args!("\n"));
    });
    ($($arg:tt)*) => ({
        $crate::tty::_print(core::format_args!("{}\n", core::format_args!($($arg)*)));
    })
}

/* ---------- Pixel + glyph helpers ---------- */

fn put_pixel(fb: &mut FrameBuffer, x: u32, y: u32, c: Rgb) {
    let info = fb.info();
    if x >= info.width as u32 || y >= info.height as u32 {
        return;
    }
    let bpp = info.bytes_per_pixel;
    let stride = info.stride;
    let idx = (y * stride as u32 + x) * bpp as u32;
    let buf = fb.buffer_mut();

    match info.pixel_format {
        PixelFormat::Rgb => {
            if bpp == 4 {
                let p = &mut buf[idx as usize..][..4];
                p[0] = c.r;
                p[1] = c.g;
                p[2] = c.b;
                p[3] = 0;
            } else {
                let p = &mut buf[idx as usize..][..3];
                p[0] = c.r;
                p[1] = c.g;
                p[2] = c.b;
            }
        }
        PixelFormat::Bgr => {
            if bpp == 4 {
                let p = &mut buf[idx as usize..][..4];
                p[0] = c.b;
                p[1] = c.g;
                p[2] = c.r;
                p[3] = 0;
            } else {
                let p = &mut buf[idx as usize..][..3];
                p[0] = c.b;
                p[1] = c.g;
                p[2] = c.r;
            }
        }
        PixelFormat::U8 => {
            let y8 =
                ((u16::from(c.r) * 30 + u16::from(c.g) * 59 + u16::from(c.b) * 11) / 100) as u8;
            buf[idx as usize] = y8;
        }
        _ => {}
    }
}

fn fill_rect(fb: &mut FrameBuffer, x: u32, y: u32, w: u32, h: u32, c: Rgb) {
    for yy in y..y + h {
        for xx in x..x + w {
            put_pixel(fb, xx, yy, c);
        }
    }
}

pub fn clear_screen(fb: &mut FrameBuffer, bg: Rgb) {
    let info = fb.info();
    fill_rect(fb, 0, 0, info.width as u32, info.height as u32, bg);
}

fn draw_glyph(
    fb: &mut FrameBuffer,
    ch: char,
    x: u32,
    y: u32,
    cell_w: u32,
    cell_h: u32,
    fg: Rgb,
    bg: Option<Rgb>,
) {
    let glyph = font8x8::legacy::BASIC_LEGACY
        .get(ch as usize)
        .unwrap_or(&[0u8; 8]);
    let sx = (cell_w / 8).max(1);
    let sy = (cell_h / 8).max(1);
    for (row, bits) in glyph.iter().enumerate() {
        for col in 0..8u32 {
            let on = (bits >> col) & 1 != 0; // flip to (7-col) if mirrored
            let px = x + col * sx;
            let py = y + (row as u32) * sy;
            match (on, bg) {
                (true, _) => fill_rect(fb, px, py, sx, sy, fg),
                (false, Some(bc)) => fill_rect(fb, px, py, sx, sy, bc),
                (false, None) => {}
            }
        }
    }
}
