#
# rename_focuser.py
# Append release version and board definition to firmware executable
#
Import("env")

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

config = configparser.ConfigParser()
config.read("platformio.ini")

env.Replace(PROGNAME="TeenAstroFocuser_%s_%s" % (config.get("env","custom_option1"), env.get("PIOENV")))
