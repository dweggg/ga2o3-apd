__attribute__((weak)) unsigned int __disable_interrupts(void)
{
    __asm(" PUSH ST1");
    __asm(" SETC INTM, DBGM");
    __asm(" POP AL");
    return 0;
}

