#![no_std]
#![no_main]

use bootloader_api::{BootInfo, entry_point};
use kernel::println;
use kernel::tty;
use kernel::tty::Rgb;

#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[unsafe(no_mangle)]
fn kernel_main(boot_info: &'static mut BootInfo) -> ! {
    if let Some(fb) = boot_info.framebuffer.as_mut() {
        tty::init_global(fb);
    }

    {
        let mut tw = tty::TW.lock();
        tw.clear(Rgb { r: 0, g: 0, b: 0 });
        tw.set_cell_size(11, 11);
        tw.set_colors(tty::Rgb { r: 0, g: 255, b: 0 }, None);
    }

    println!("-----------------------------------");
    println!("Welcome to ZirconOS!");
    println!("Initialized kernel at: 0x{:x}", boot_info.kernel_addr);
    println!("");
    println!("Version 0.0.1");
    println!("-----------------------------------");
    println!("");

    loop {}
}

const CONFIG: bootloader_api::BootloaderConfig = {
    let mut config = bootloader_api::BootloaderConfig::new_default();
    config.kernel_stack_size = 100 * 1024; // 100 KiB
    config
};

entry_point!(kernel_main, config = &CONFIG);
