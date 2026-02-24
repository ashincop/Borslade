import shutil
shutil.copy2(input("Please enter a path to the OVMF Code: "), 'immu/OVMF_CODE.fd')
shutil.copy2(input("Please enter a path to the OVMF Vars: "), 'immu/OVMF_VARS.fd')