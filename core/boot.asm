extern start_kernel
; Multiboot2 header (32-bit) requesting a 1280x720 framebuffer @ 32bpp
align 8
multiboot2_header:
	dd 0xE85250D6        ; multiboot2 magic
	dd 0x0               ; architecture: 0 = i386
	dd 48                ; header_length (bytes)
	dd 0x17ADAEFA        ; checksum (magic + arch + length + checksum = 0)

	; Framebuffer request tag
	dd 8                 ; tag type: framebuffer
	dd 24                ; tag size (including padding to 8-byte alignment)
	dd 1280              ; width
	dd 720               ; height
	dd 32                ; depth (bits per pixel)
	dd 0                 ; padding to 8-byte alignment

	; End tag
	dd 0
	dd 8
call start_kernel