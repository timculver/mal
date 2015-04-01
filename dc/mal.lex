/* Token representation:
   1 [foo] 102 111 111   the symbol `foo`
   2 [bar]               "bar"
   3                     ~@
   40                    (
   41                    )
   etc.
*/

  FILE *dc = NULL;

%%

[\n] { 
  fprintf(dc, "lRxc\n"); /* trigger rep() */
  while (1) {
    int c = fgetc(dc);
    if (c == EOF) {
      clearerr(dc); break;
      //exit(0);
    }
    if (c == '\26')
      break;
    fputc(c, stdout);
  }
  fflush(stdout);
  fprintf(stdout, "\nuser> ");
  fflush(stdout);
} 

[ \t,]+ /* whitespace */

;.* /* comment */

~@ { fprintf(dc, "3\n"); }

[\[\]{}()'`~^@] { fprintf(dc, "%d\n", yytext[0]); }

\"(\?:\\.|[^\\"])*\" { 
  /* String. In order to be printed by dc, a string must be a valid dc macro.
     This means that square brackets [] must be balanced. I don't know of any
     other restrictions. */
  int len_including_quotes = strlen(yytext);
  yytext[len_including_quotes - 1] = '\0';
  fprintf(dc, "2 [%s]\n", yytext + 1); 
}

[^ \t\r\n\[\]{}('"`,;)]* { 
  fprintf(dc, "1 [%s] ", yytext);
  for (char* c = yytext; *c; c++)
    fprintf(dc, "%d ", *c);
  fprintf(dc, "\n");
}

%%

int main(int argc, char **argv) {
  int ch;
  int use_dc = 1;
  while ((ch = getopt(argc, argv, "t")) != -1) {
    switch (ch) {
    case 't':
      use_dc = 0;
      break;
    default:
      printf("usage: %s [-t]\n -t: tokens only; don't start dc\n", argv[0]);
      exit(0);
    }
  }
  argc -= optind;
  argv += optind;

  if (use_dc)
    dc = popen("dc types.dc step0_repl.dc -", "r+");
  else
    dc = stdout;
  ++argv, --argc;  /* skip over program name */
  if ( argc > 0 )
    yyin = fopen( argv[0], "r" );
  else
    yyin = stdin;
  fprintf(stdout, "user> ");
  yylex();
  pclose(dc);
}