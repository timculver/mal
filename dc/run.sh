lex mal.lex
cc -L/usr/local/opt/flex/lib -I/usr/local/opt/flex/include \
  lex.yy.c -lfl -o mal
