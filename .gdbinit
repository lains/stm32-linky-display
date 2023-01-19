handle SIGTRAP nostop noprint
target remote :3333
monitor reset
load
break main
continue
