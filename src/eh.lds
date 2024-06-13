SECTIONS {
    .eh_init_fini_array :
    {
        PROVIDE_HIDDEN (__start_eh_init_fini_array = .);
        KEEP (*(.eh_init_fini_array.modeule_call_0))
        KEEP (*(.eh_init_fini_array.modeule_call_1))
        KEEP (*(.eh_init_fini_array.modeule_call_2))
        KEEP (*(.eh_init_fini_array.modeule_call_3))
        KEEP (*(.eh_init_fini_array.modeule_call_4))
        KEEP (*(.eh_init_fini_array.modeule_call_5))
        KEEP (*(.eh_init_fini_array.modeule_call_6))
        KEEP (*(.eh_init_fini_array.modeule_call_7))
        PROVIDE_HIDDEN (__end_eh_init_fini_array = .);
    }
}
INSERT AFTER .rodata;

SECTIONS {
    .eh_init_fini :
    {
        PROVIDE_HIDDEN (__start_eh_init_fini = .);
        KEEP (*(.eh_init_fini*))
        PROVIDE_HIDDEN (__end_eh_init_fini = .);
    }
}
INSERT AFTER .text;