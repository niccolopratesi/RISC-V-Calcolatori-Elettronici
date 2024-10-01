.global read_write_stopei
read_write_stopei:
  csrrw a0, stopei, x0
  ret
