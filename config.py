#!/usr/bin/env python3
import kconfiglib
from menuconfig import menuconfig

kconf = kconfiglib.Kconfig("Kconfig")
menuconfig(kconf)
