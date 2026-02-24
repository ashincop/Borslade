## How to build Borslade

Execute make menuconfig, choose your options as documented (not yet, self descriptive for now).

Execute init.py, it requires no dependencies (note: this step is optional and requires a further step).

After init.py, run "make run" or "make" to run/compile the kernel

If you used init.py, you will realize you popped in the shell.

After, type 'bcfg boot add 0 fs0:\efi\boot\bootx64.efi "Borslade"'

Next, type 'reset'

Now you are ready to continue building your version of your kernel, edit it, contribute!

Good Luck!