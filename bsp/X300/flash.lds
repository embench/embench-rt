/* Copyright(C) 2019 Hex Five Security, Inc. */
/* 10-MAR-2019 Cesare Garlati                */                 

OUTPUT_ARCH("riscv")

ENTRY(start)

MEMORY {
  flash (rxai!w) : ORIGIN = 0x20400000, LENGTH = 64K
  ram   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 1K  
}

SECTIONS {
  .text : { *(.text .text.*) } >flash 
  .data : { *(.data .data.*) } >ram
}
