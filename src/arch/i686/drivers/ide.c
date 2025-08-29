#include "arch/i686/drivers/ide.h"

#include <arch/i686/io.h>
#include <kernel/sleep.h>
#include <stdio.h>

// ---------- Globals ----------
unsigned char ide_buf[2048] = {0};
static volatile unsigned char ide_irq_invoked = 0;
static unsigned char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned char package[2] = {0};

IDEChannelRegisters channels[2];
ide_device_t ide_devices[4];

void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    i686_outb(channels[channel].base + reg - 0x00, data);
  else if (reg < 0x0C)
    i686_outb(channels[channel].base + reg - 0x06, data);
  else if (reg < 0x0E)
    i686_outb(channels[channel].ctrl + reg - 0x0A, data);
  else if (reg < 0x16)
    i686_outb(channels[channel].bmide + reg - 0x0E, data);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_read(unsigned char channel, unsigned char reg) {
  unsigned char result;
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    result = i686_inb(channels[channel].base + reg - 0x00);
  else if (reg < 0x0C)
    result = i686_inb(channels[channel].base + reg - 0x06);
  else if (reg < 0x0E)
    result = i686_inb(channels[channel].ctrl + reg - 0x0A);
  else if (reg < 0x16)
    result = i686_inb(channels[channel].bmide + reg - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
  return result;
}

void ide_read_buffer(unsigned char channel, unsigned char reg,
                     unsigned int buffer, unsigned int quads) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL,
              (uint8_t)(0x80 | channels[channel].nIEN));

  uint16_t port;
  if (reg < 0x08)
    port = (uint16_t)(channels[channel].base + reg - 0x00);
  else if (reg < 0x0C)
    port = (uint16_t)(channels[channel].base + reg - 0x06);
  else if (reg < 0x0E)
    port = (uint16_t)(channels[channel].ctrl + reg - 0x0A);
  else if (reg < 0x16)
    port = (uint16_t)(channels[channel].bmide + reg - 0x0E);
  else
    return;
  i686_insl(port, (void *)buffer, quads);

  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check) {

  for (int i = 0; i < 4; i++)
    ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port
                                          // wastes 100ns; loop four times.

  while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ; // Wait for BSY to be zero.

  if (advanced_check) {
    unsigned char state =
        ide_read(channel, ATA_REG_STATUS); // Read Status Register.

    if (state & ATA_SR_ERR)
      return 2; // Error.

    if (state & ATA_SR_DF)
      return 1; // Device Fault.

    // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
    if ((state & ATA_SR_DRQ) == 0)
      return 3; // DRQ should be set
  }

  return 0; // No Error.
}

unsigned char ide_print_error(unsigned int drive, unsigned char err) {
  if (err == 0)
    return err;

  printf("IDE:");
  if (err == 1) {
    printf("- Device Fault\n     ");
    err = 19;
  } else if (err == 2) {
    unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
    if (st & ATA_ER_AMNF) {
      printf("- No Address Mark Found\n     ");
      err = 7;
    }
    if (st & ATA_ER_TK0NF) {
      printf("- No Media or Media Error\n     ");
      err = 3;
    }
    if (st & ATA_ER_ABRT) {
      printf("- Command Aborted\n     ");
      err = 20;
    }
    if (st & ATA_ER_MCR) {
      printf("- No Media or Media Error\n     ");
      err = 3;
    }
    if (st & ATA_ER_IDNF) {
      printf("- ID mark not Found\n     ");
      err = 21;
    }
    if (st & ATA_ER_MC) {
      printf("- No Media or Media Error\n     ");
      err = 3;
    }
    if (st & ATA_ER_UNC) {
      printf("- Uncorrectable Data Error\n     ");
      err = 22;
    }
    if (st & ATA_ER_BBK) {
      printf("- Bad Sectors\n     ");
      err = 13;
    }
  } else if (err == 3) {
    printf("- Reads Nothing\n     ");
    err = 23;
  } else if (err == 4) {
    printf("- Write Protected\n     ");
    err = 8;
  }
  printf(
      "- [%s %s] %s\n",
      (const char *[]){
          "Primary",
          "Secondary"}[ide_devices[drive].channel], // Use the channel as an
                                                    // index into the array
      (const char *[]){
          "Master",
          "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
      ide_devices[drive].model);

  return err;
}

void ide_wait_irq() {
  while (!ide_irq_invoked)
    ;
  ide_irq_invoked = 0;
}

void ide_irq() { ide_irq_invoked = 1; }

void ide_init(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2,
              unsigned int BAR3, unsigned int BAR4) {
  int j, k, i, count = 0;

  channels[ATA_PRIMARY].base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
  channels[ATA_PRIMARY].ctrl = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
  channels[ATA_SECONDARY].base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
  channels[ATA_SECONDARY].ctrl = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
  channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0;   // Bus Master IDE
  channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

  // Disable IRQ
  ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
  ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++) {

      unsigned char err = 0, type = IDE_ATA, status;
      ide_devices[count].reserved = 0; // Assuming that no drive here.

      ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
      sleep(1);

      ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
      sleep(1);

      if (ide_read(i, ATA_REG_STATUS) == 0)
        continue; // If Status = 0, No Device.

      while (1) {
        status = ide_read(i, ATA_REG_STATUS);
        if ((status & ATA_SR_ERR)) {
          err = 1;
          break;
        } // If Err, Device is not ATA.
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
          break; // Everything is right.
      }

      if (err != 0) {
        unsigned char cl = ide_read(i, ATA_REG_LBA1);
        unsigned char ch = ide_read(i, ATA_REG_LBA2);

        if (cl == 0x14 && ch == 0xEB)
          type = IDE_ATAPI;
        else if (cl == 0x69 && ch == 0x96)
          type = IDE_ATAPI;
        else
          continue; // Unknown Type (may not be a device).

        ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
        sleep(1);
      }

      ide_read_buffer(i, ATA_REG_DATA, (unsigned int)ide_buf, 128);

      ide_devices[count].reserved = 1;
      ide_devices[count].type = type;
      ide_devices[count].channel = i;
      ide_devices[count].drive = j;
      ide_devices[count].signature =
          *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
      ide_devices[count].capabilities =
          *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
      ide_devices[count].command_sets =
          *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));

      if (ide_devices[count].command_sets & (1 << 26))
        // Device uses 48-Bit Addressing:
        ide_devices[count].size =
            *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
      else
        // Device uses CHS or 28-bit Addressing:
        ide_devices[count].size =
            *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));

      for (k = 0; k < 40; k += 2) {
        ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
        ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
      }
      ide_devices[count].model[40] = 0; // Terminate String.

      count++;
    }

  for (i = 0; i < 4; i++)
    if (ide_devices[i].reserved == 1) {
      printf(" Found %s Drive %d Kib - %s\n",
             (const char *[]){"ATA", "ATAPI"}[ide_devices[i].type], /* Type */
             ide_devices[i].size / 2,                               /* Size */
             ide_devices[i].model);
    }
}

static inline void ata_400ns_delay(unsigned char channel) {
  (void)ide_read(channel, ATA_REG_ALTSTATUS);
  (void)ide_read(channel, ATA_REG_ALTSTATUS);
  (void)ide_read(channel, ATA_REG_ALTSTATUS);
  (void)ide_read(channel, ATA_REG_ALTSTATUS);
}

unsigned char ide_ata_access(unsigned char direction, unsigned char drive,
                             unsigned int lba, unsigned char numsects,
                             unsigned short selector, unsigned int edi) {
  unsigned char lba_mode, dma, cmd;
  unsigned char lba_io[6];
  unsigned int channel = ide_devices[drive].channel;
  unsigned int slavebit = ide_devices[drive].drive;
  unsigned int bus = channels[channel].base;
  unsigned int words = 256;
  unsigned short cyl, i;
  unsigned char head, sect, err;

  if (numsects == 0)
    numsects = 1; // be explicit

  // disable IRQ
  ide_write(channel, ATA_REG_CONTROL,
            channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);

  if (lba >= 0x10000000) {
    // LBA48
    lba_mode = 2;
    lba_io[0] = (lba & 0x000000FF) >> 0;
    lba_io[1] = (lba & 0x0000FF00) >> 8;
    lba_io[2] = (lba & 0x00FF0000) >> 16;
    lba_io[3] = (lba & 0xFF000000) >> 24;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = 0;
  } else if (ide_devices[drive].capabilities & 0x200) {
    // LBA28  (FIXED MASKS)
    lba_mode = 1;
    lba_io[0] = (lba & 0x000000FF) >> 0;
    lba_io[1] = (lba & 0x0000FF00) >> 8;
    lba_io[2] = (lba & 0x00FF0000) >> 16;
    lba_io[3] = 0;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = (unsigned char)((lba >> 24) & 0x0F);
  } else {
    // CHS
    lba_mode = 0;
    sect = (lba % 63) + 1;
    cyl = (lba + 1 - sect) / (16 * 63);
    lba_io[0] = sect;
    lba_io[1] = (cyl >> 0) & 0xFF;
    lba_io[2] = (cyl >> 8) & 0xFF;
    lba_io[3] = 0;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = (unsigned char)(((lba + 1 - sect) / 63) % 16);
  }

  dma = 0; // PIO

  // Wait for BSY clear
  while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY) {
  }

  if (lba_mode == 0)
    ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head);
  else
    ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);

  ata_400ns_delay(channel); // <<< REQUIRED

  for (;;) {
    unsigned char s = ide_read(channel, ATA_REG_STATUS);
    if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRDY))
      break;
  }

  // Program taskfile
  if (lba_mode == 2) {
    ide_write(channel, ATA_REG_SECCOUNT1, 0);
    ide_write(channel, ATA_REG_LBA3, lba_io[3]);
    ide_write(channel, ATA_REG_LBA4, lba_io[4]);
    ide_write(channel, ATA_REG_LBA5, lba_io[5]);
  }
  ide_write(channel, ATA_REG_SECCOUNT0, numsects);
  ide_write(channel, ATA_REG_LBA0, lba_io[0]);
  ide_write(channel, ATA_REG_LBA1, lba_io[1]);
  ide_write(channel, ATA_REG_LBA2, lba_io[2]);

  if (lba_mode == 0 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO_EXT;
  if (lba_mode == 0 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO_EXT;

  ide_write(channel, ATA_REG_COMMAND, cmd);

  if (dma) {
    // (not used)
  } else if (direction == 0) {
    // READ PIO
    for (i = 0; i < numsects; i++) {
      if ((err = ide_polling(channel, 1)))
        return err; // will return 3 if DRQ never set
      // safer asm: ensure DF=0 and tell the compiler EDI/ECX are modified
      asm volatile("cld; rep insw"
                   : "+D"(edi)
                   : "c"(words), "d"(bus)
                   : "memory");
    }
  } else {
    // WRITE PIO
    for (i = 0; i < numsects; i++) {
      ide_polling(channel, 0);
      asm volatile("cld; rep outsw"
                   :
                   : "S"(edi), "c"(words), "d"(bus)
                   : "memory");
      edi += words * 2;
    }
    ide_write(channel, ATA_REG_COMMAND,
              (char[]){ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH,
                       ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
    ide_polling(channel, 0);
  }

  return 0;
}

unsigned char ide_atapi_read(unsigned char drive, unsigned int lba,
                             unsigned char numsects, unsigned short selector,
                             unsigned int edi) {
  unsigned int channel = ide_devices[drive].channel;
  unsigned int slavebit = ide_devices[drive].drive;
  unsigned int bus = channels[channel].base;
  unsigned int words =
      1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
  unsigned char err;
  int i;

  ide_write(channel, ATA_REG_CONTROL,
            channels[channel].nIEN = ide_irq_invoked = 0x0); // enable irq

  atapi_packet[0] = ATAPI_CMD_READ;
  atapi_packet[1] = 0x0;
  atapi_packet[2] = (lba >> 24) & 0xFF;
  atapi_packet[3] = (lba >> 16) & 0xFF;
  atapi_packet[4] = (lba >> 8) & 0xFF;
  atapi_packet[5] = (lba >> 0) & 0xFF;
  atapi_packet[6] = 0x0;
  atapi_packet[7] = 0x0;
  atapi_packet[8] = 0x0;
  atapi_packet[9] = numsects;
  atapi_packet[10] = 0x0;
  atapi_packet[11] = 0x0;

  ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

  for (int i = 0; i < 4; i++)
    ide_read(
        channel,
        ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns.

  ide_write(channel, ATA_REG_FEATURES, 0);

  ide_write(channel, ATA_REG_LBA1,
            (words * 2) & 0xFF); // Lower Byte of Sector Size.
  ide_write(channel, ATA_REG_LBA2,
            (words * 2) >> 8); // Upper Byte of Sector Size.

  ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET); // Send the Command.

  if (err = ide_polling(channel, 1))
    return err; // Polling and return if error.

  asm("rep   outsw"
      :
      : "c"(6), "d"(bus), "S"(atapi_packet)); // Send Packet Data

  for (i = 0; i < numsects; i++) {
    ide_wait_irq(); // Wait for an IRQ.
    if (err = ide_polling(channel, 1))
      return err; // Polling and return if error.
    asm("pushw %es");
    asm("mov %%ax, %%es" ::"a"(selector));
    asm("rep insw" ::"c"(words), "d"(bus), "D"(edi)); // Receive Data.
    asm("popw %es");
    edi += (words * 2);
  }

  ide_wait_irq();

  while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
    ;

  return 0;
}

void ide_read_sectors(unsigned char drive, unsigned char numsects,
                      unsigned int lba, unsigned short es, unsigned int edi) {

  int i;
  if (drive > 3 || ide_devices[drive].reserved == 0)
    package[0] = 0x1; // Drive Not Found!

  else if (((lba + numsects) > ide_devices[drive].size) &&
           (ide_devices[drive].type == IDE_ATA))
    package[0] = 0x2; // Seeking to invalid position.

  else {
    unsigned char err;
    if (ide_devices[drive].type == IDE_ATA)
      err = ide_ata_access(ATA_READ, drive, lba, numsects, es, edi);
    else if (ide_devices[drive].type == IDE_ATAPI)
      for (i = 0; i < numsects; i++)
        err = ide_atapi_read(drive, lba + i, 1, es, edi + (i * 2048));
    package[0] = ide_print_error(drive, err);
  }
}

void ide_write_sectors(unsigned char drive, unsigned char numsects,
                       unsigned int lba, unsigned short es, unsigned int edi) {

  if (drive > 3 || ide_devices[drive].reserved == 0)
    package[0] = 0x1; // Drive Not Found!

  else if (((lba + numsects) > ide_devices[drive].size) &&
           (ide_devices[drive].type == IDE_ATA))
    package[0] = 0x2; // Seeking to invalid position.

  else {
    unsigned char err;
    if (ide_devices[drive].type == IDE_ATA)
      err = ide_ata_access(ATA_WRITE, drive, lba, numsects, es, edi);
    else if (ide_devices[drive].type == IDE_ATAPI)
      err = 4; // Write-Protected.
    package[0] = ide_print_error(drive, err);
  }
}

void ide_atapi_eject(unsigned char drive) {
  unsigned int channel = ide_devices[drive].channel;
  unsigned int slavebit = ide_devices[drive].drive;
  unsigned int bus = channels[channel].base;
  unsigned int words = 2048 / 2; // Sector Size in Words.
  unsigned char err = 0;
  ide_irq_invoked = 0;

  if (drive > 3 || ide_devices[drive].reserved == 0)
    package[0] = 0x1; // Drive Not Found!
  else if (ide_devices[drive].type == IDE_ATA)
    package[0] = 20; // Command Aborted.
  else {
    // Enable IRQs:
    ide_write(channel, ATA_REG_CONTROL,
              channels[channel].nIEN = ide_irq_invoked = 0x0);

    atapi_packet[0] = ATAPI_CMD_EJECT;
    atapi_packet[1] = 0x00;
    atapi_packet[2] = 0x00;
    atapi_packet[3] = 0x00;
    atapi_packet[4] = 0x02;
    atapi_packet[5] = 0x00;
    atapi_packet[6] = 0x00;
    atapi_packet[7] = 0x00;
    atapi_packet[8] = 0x00;
    atapi_packet[9] = 0x00;
    atapi_packet[10] = 0x00;
    atapi_packet[11] = 0x00;

    ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    for (int i = 0; i < 4; i++)
      ide_read(
          channel,
          ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET); // Send the Command.

    err = ide_polling(channel, 1); // Polling and stop if error.
    asm("rep   outsw" ::"c"(6), "d"(bus),
        "S"(atapi_packet));        // Send Packet Data
    ide_wait_irq();                // Wait for an IRQ.
    err = ide_polling(channel, 1); // Polling and get error code.
    if (err == 3)
      err = 0; // DRQ is not needed here.
  }
  package[0] = ide_print_error(drive, err); // Return;
}