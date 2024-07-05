SECTIONS {
    .eh_init_fini_array :
    {
        . = ALIGN(8);
        PROVIDE_HIDDEN (__start_eh_init_fini_array = .);
        KEEP (*(SORT(.eh_init_fini_array.*)));
        PROVIDE_HIDDEN (__end_eh_init_fini_array = .);
    }
}
INSERT AFTER .rodata;