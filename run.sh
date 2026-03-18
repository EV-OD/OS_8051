sdas8051 -los  "$1.asm"
sdld -i "$1.rel"
packihx "$1.ihx" > "$1.hex"
