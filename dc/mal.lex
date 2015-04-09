/* Token representation:
   0 77                  the number 77
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
      clearerr(dc);
      break;
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
	yytext++;
  fprintf(dc, "2 [%s]", yytext); 
  for (char* c = yytext; *c; c++)
    fprintf(dc, " %d", *c);
  fprintf(dc, "\n");
}

[0-9]+ {
  fprintf(dc, "0 %s\n", yytext);
}

[^ \t\r\n\[\]{}('"`,;)~]* { 
  fprintf(dc, "1 [%s]", yytext);
  for (char* c = yytext; *c; c++)
    fprintf(dc, " %d", *c);
  fprintf(dc, "\n");
}

%%

void usage(char* progname) {
  printf("usage: %s [-t] stepN_whatever.dc\n -t: tokens only; don't start dc\n", progname);
  exit(0);
}

int main(int argc, char **argv) {
  int ch;
  int use_dc = 1;
  char* progname = argv[0];
  while ((ch = getopt(argc, argv, "t")) != -1) {
    switch (ch) {
    case 't':
      use_dc = 0;
      break;
    default:
      usage(progname);
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0)
    usage(progname);
  
  char* dcfile = argv[0];

  if (use_dc) {
    char cmd[4096];
    snprintf(cmd, 4096, "dc types.dc reader.dc printer.dc init.dc %s - 2>&1", dcfile);
    dc = popen(cmd, "r+");
  } else {
    dc = stdout;
  }
  ++argv, --argc;  /* skip over program name */
  yyin = stdin;
  fprintf(stdout, "user> ");
  yylex();
  pclose(dc);
}