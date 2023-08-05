/*
 * Copyright (c) 2023, Arteen Abrishami. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *        
 * All advertising materials mentioning features or use of this software must
 * display the following acknowledgement: This product includes software
 * developed by Arteen Abrishami.
 *
 * Neither the name of Arteen Abrishami nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *        
 * THIS SOFTWARE IS PROVIDED BY ARTEEN ABRISHAMI AS IS AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*  ================ INCLUDES  ================ */

/* https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
 */
#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>

/*  ================ DEFINES  ================ */

#define LE_VERSION "0.0.1"

/*  ================ screen management  ================ */

/* https://stackoverflow.com/questions/39188508/how-curses-preserves
 * -screen-contents */
#define ENABLE_ALT_SCREEN "\x1b[?1049h"
#define DISABLE_ALT_SCREEN "\x1b[?1049l"
#define ENABLE_ALT_SCREEN_SZ 8
#define DISABLE_ALT_SCREEN_SZ 8
/* from the obscure depths of the internet
 * https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
 */
#define ENABLE_MOUSE_TRACKING "\x1b[?1000h"
#define DISABLE_MOUSE_TRACKING "\x1b[?1000l"
#define ENABLE_MOUSE_TRACKING_SZ 8
#define DISABLE_MOUSE_TRACKING_SZ 8
/* http://vt100.net/docs/vt510-rm/DECTCEM.html */
#define HIDE_CURSOR "\x1b[?25l"
#define HIDE_CURSOR_SZ 6
#define UNHIDE_CURSOR "\x1b[?25h"
#define UNHIDE_CURSOR_SZ 6
/* https://vt100.net/docs/vt100-ug/chapter3.html#ED */
#define ERASE_DISPLAY "\x1b[2J"
#define ERASE_DISPLAY_SZ 4
/* https://vt100.net/docs/vt100-ug/chapter3.html#CUP */
#define MV_CURSOR_TOP_LEFT "\x1b[H"
#define MV_CURSOR_TOP_LEFT_SZ 3
/* https://vt100.net/docs/vt100-ug/chapter3.html#CUP */
#define MV_CURSOR_COORD_ARGS_YX "\x1b[%d;%dH"
/* https://vt100.net/docs/vt100-ug/chapter3.html#CUD */
/* https://vt100.net/docs/vt100-ug/chapter3.html#CUF */
#define MV_CURSOR_BOT_RIGHT "\x1b[999C\x1b[999B"
#define MV_CURSOR_BOT_RIGHT_SZ 12
/* https://vt100.net/docs/vt100-ug/chapter3.html#DSR */
#define GET_CURSOR_POS "\x1b[6n"
#define GET_CURSOR_POS_SZ 4
/* https://vt100.net/docs/vt100-ug/chapter3.html#EL */
#define ERASE_TO_EOL "\x1b[K"
#define ERASE_TO_EOL_SZ 3
/* due to -opost */
#define EOL "\r\n"
#define EOL_SZ 2
/* https://vt100.net/docs/vt100-ug/chapter3.html#SGR */
#define START_INVERT_TEXT "\x1b[7m"
#define START_INVERT_TEXT_SZ 4
#define END_INVERT_TEXT "\x1b[m"
#define END_INVERT_TEXT_SZ 3

/* ================ key bindings ================ */

#ifndef CTRL
#define CTRL(c) (c & 0x1f)
#endif

#define FORWARD_CHAR CTRL('F')
#define BACKWARD_CHAR CTRL('B')

#define NEXT_LINE CTRL('N')
#define PREV_LINE CTRL('P')

#define MV_BEG_OF_LINE CTRL('A')
#define MV_END_OF_LINE CTRL('E')

#define SCROLL_DOWN CTRL('V') /* or PAGE DOWN (<fn>+<key down> macOS) */

#define SCROLL_UP 1000 /* M-v or PAGE UP (<fn>+<key up> macOS) */

#define BEG_OF_BUF 1001 // M-< or HOME (<fn>+<key left> macOS)
#define END_OF_BUF 1002 // M-> or END (<fn>+<key right> macOS)

#define DEL_FORWARD_CHAR 1003 // delete (fn+<delete> on macOS)
#define DEL_BACKWARD_CHAR 127 // backspace (<delete> on macOS)

/* ================ initializers ================ */

#define ABUF_INIT { 0, NULL }

/* ================ misc ================ */

#define TAB_STOP_SZ 4

#define DIE_ERROR_FMT "%s: %s with message '%s'\n"
#define DIE_MSG_FMT "%s: %s\n"

/* ================ LOG (optional) ================ */

#define LOG

#ifdef LOG

#include <fcntl.h>

#define INIT_LOG(f) do {                                    \
	int fd = open(f, O_CREAT | O_WRONLY | O_TRUNC, 0644);    \
	if (fd == -1) exit(79);									\
	fd = dup2(fd, STDERR_FILENO);							\
	if (fd == -1) exit(79);									\
  } while (0)

#define WRITE_LOG_DELIM(s) do {                         \
	fprintf(stderr, "\n|---- %s ----|\n\n", s);			\
  } while (0)

#define WRITE_LOG(s) do {						\
	fprintf(stderr, "%s\n", s);					\
  } while (0)

#define WRITE_LOG_STR(s, l) do {					\
	fprintf(stderr, "%.s\n", s, l);					\
  } while (0)

#define WRITE_LOG_INT(s, i) do {				\
	fprintf(stderr,"%s: %d\n", s, i);			\
  } while (0)

#define WRITE_LOG_CHAR(c) do {									\
  if (iscntrl(c))													\
	fprintf(stderr, "char: %d ('^%c')\n", c, c + 'A' - 1);          \
  else																\
	fprintf(stderr, "char: %d ('%c')\n", c, c);                     \
  } while (0)

#define WRITE_LOG_PTR(s, p) do {                  \
    fprintf(stderr, "%s: %p\n", s, (void*) p);     \
  } while (0)

#else

#define INIT_LOG(f)
#define DELIM_WRITE_LOG(s)
#define WRITE_LOG(s)
#define WRITE_LOG_STR(s, l)
#define WRITE_LOG_INT(s, i)
#define WRITE_LOG_CHAR(s, c)
#define WRITE_LOG_PTR(s, p)

#endif

/* ================ GLOBALS ================ */

/* ================ editor state ================ */

struct editor_row
{
  /* size of actual chars present in row */
  int size;
  /* size of the rendered chars on screen */
  int rsize;
  /* the actual chars */
  char *chars;
  /* the rendered chars */
  char *render;
};

struct editor_state_struct
{
  /* restore upon exit */
  struct termios orig_termios;
  /* a row in our editor */
  struct editor_row *row;
  /* the filename we are responsible for */
  char *filename;
  /* our status bar msg (bottom bar) */
  char status_msg[80];
  /* keep track and remove it when necessary */
  time_t status_msg_time;
  /* how many rows do we have of text */
  int num_rows;
  /* how many rows up top are we missing (scrolling) */
  int row_offset;
  /* how many cols to the left missing (scrolling) */
  int col_offset;

  /* cursor position -- within the chars field of the editor rows */
  int cx, cy;
  /* cursor position -- within the render field of editor rows,
     adjusted for tabs, adjust for real cursor pos */
  int rx;
  /* for the terminal window */
  int window_rows, window_cols;

  /* where do I say it's End of buffer */
  // bool final_row_newline;
} editor;

const char *progname;

/* ================ append buffer ================ */

struct abuf
{
  int len;
  char *buf;
} ab = ABUF_INIT;

/* ================ FUNCTIONS ================ */

/* ================ abuf related ================ */

void
abuf_append(const char *s, int len)
{
  char *new_buf = realloc(ab.buf, ab.len + len);
  if (new_buf == NULL)
	return;
  memcpy(&new_buf[ab.len], s, len);
  ab.buf = new_buf;
  ab.len += len;
}

void
abuf_destruct(void)
{
  free(ab.buf);
}

/* ================ misc ================ */

[[ noreturn ]]
void
die(const char *fmt, const char *s)
{
  const char *error = strerror(errno);
  fprintf(stderr, fmt, progname, s, error);
  exit(EXIT_FAILURE);
}

void
editor_clear_screen(void)
{
  if (
	  write(STDOUT_FILENO, MV_CURSOR_TOP_LEFT,
			MV_CURSOR_TOP_LEFT_SZ) == -1
      ||
	  write(STDOUT_FILENO, ERASE_DISPLAY,
			ERASE_DISPLAY_SZ) == -1
	  )
	die(DIE_ERROR_FMT, "write");
}

/* ================ terminal control ================ */

void
disable_raw_mode(void)
{
  /* not gonna call "die" to exit in an atexit handler */
  write(STDOUT_FILENO, DISABLE_ALT_SCREEN DISABLE_MOUSE_TRACKING,
      DISABLE_ALT_SCREEN_SZ + DISABLE_MOUSE_TRACKING_SZ);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.orig_termios);
}

void
enable_raw_mode(void)
{
  if (! isatty(STDIN_FILENO) || ! isatty(STDOUT_FILENO)
      || tcgetattr(STDIN_FILENO, &editor.orig_termios) == -1)
	die(DIE_MSG_FMT, "stdin and stdout must be terminal devices");
  atexit(disable_raw_mode);
  
  if (write(STDOUT_FILENO, ENABLE_ALT_SCREEN ENABLE_MOUSE_TRACKING,
        ENABLE_ALT_SCREEN_SZ + ENABLE_MOUSE_TRACKING_SZ)
      != ENABLE_ALT_SCREEN_SZ + ENABLE_MOUSE_TRACKING_SZ)
    die(DIE_MSG_FMT, "internal write error");
      
  
  /* https://man7.org/linux/man-pages/man3/termios.3.html */
  struct termios raw = editor.orig_termios;
  raw.c_iflag &= ~(BRKINT | INPCK | PARMRK | INLCR | IGNCR | ISTRIP | ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cflag &= ~(CSIZE | PARENB);
  raw.c_cflag |= (CS8);

  /* don't block longer than 1/10 sec for reads */
  /* so we can detect escape sequences correctly */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	die(DIE_ERROR_FMT, "failed setting terminal attributes");
}


int
read_n(char *cp, int n)
{
  int nread;
  nread = read(STDIN_FILENO, cp, n);
  if (nread != -1)
    return nread;
  die(DIE_ERROR_FMT, "failed reading input");
}

int
editor_read_key(void)
{
  char c;
  
  while ( read_n(&c, 1) != 1 )
    ;
  
  if (c == '\x1b')
	{
	  char seq[3];

	  /* escape or meta (read timed out) */
	  if (read_n(&seq[0], 1) != 1)
        return '\x1b';

      /* meta key bindings */
      switch(seq[0])
        {
        case 'v':
          return SCROLL_UP;
        case '<':
          return BEG_OF_BUF;
        case '>':
          return END_OF_BUF;
        }
	  
	  if (read_n(&seq[1], 1) != 1)
        return '\x1b';


	  if (seq[0] == '[')
		{
		  if (seq[1] >= '0' && seq[1] <= '9')
			{
			  if (read_n(&seq[2], 1) != 1)
                return '\x1b';
              
			  if (seq[2] == '~')
				switch (seq[1])
				  {
					/* page up and down keys */
					/* caught on MacOS Terminal.app (fn+<keyup/down>) */
				  case '5':
					return SCROLL_UP;
				  case '6':
					return SCROLL_DOWN;
				  case '1':
				  case '7':
					return BEG_OF_BUF;
				  case '4':
				  case '8':
					return END_OF_BUF;
				  case '3':
					return DEL_FORWARD_CHAR;
				  }
			}
		  else
            /* ABCD -> arrow keys */
            /* H F -> possible HOME/END */
			switch (seq[1])
			  {
			  case 'A':
				return PREV_LINE;
			  case 'B':
				return NEXT_LINE;
			  case 'C':
				return FORWARD_CHAR;
			  case 'D':
				return BACKWARD_CHAR;
			  case 'H':
				return BEG_OF_BUF;
			  case 'F':
				return END_OF_BUF;
              case 'M':
                {
                  /* scrolling with term mode 1000 */
                  char scroll[3];
                  if (read_n(scroll, 3) != 3)
                    return '\x1b';
                  if (scroll[0] == 96)
                    return PREV_LINE;
                  else if (scroll[0] == 97)
                    return NEXT_LINE;
                  else
                    return '\x1b';
                }
			  }
		}
	  else if (seq[0] == '0')
        /* possible HOME/END  */
		switch (seq[1])
		  {
		  case 'H':
			return BEG_OF_BUF;
		  case 'F':
			return END_OF_BUF;
		  }
      
	  /* otherwise they just hit escape */
	  return '\x1b';
	}

  return c;
}

int
get_cursor_position(int *rows, int *cols)
{
  char buf[32];
  size_t i = 0;

  if (write(STDOUT_FILENO, GET_CURSOR_POS,
			GET_CURSOR_POS_SZ) != 4)
	return -1;

  int ret;
  while (i < sizeof(buf) - 1)
	{
	  if ( (ret = read(STDIN_FILENO, &buf[i], 1))
		   != 1
		   ||
		   buf[i] == 'R'
		   )
		break;
	  i++;
	}
  
  if (ret == -1)
	die(DIE_ERROR_FMT, "read");

  buf[i] = '\0';

  if (buf[0] != '\x1b'
	  || sscanf(&buf[1], "[%d;%d", rows, cols) != 2)
	return -1;

  return 0;
}

int
get_window_size(int *rows, int *cols)
{
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1
	  ||
	  ws.ws_col == 0
	  )
	{
	  /* backup method */
	  if (
		  write(STDOUT_FILENO, MV_CURSOR_BOT_RIGHT,
				MV_CURSOR_BOT_RIGHT_SZ) != 12
		  )
		return -1;
	  return get_cursor_position(rows, cols);
	}
  else
	{
	  *cols = ws.ws_col;
	  *rows = ws.ws_row;
	  return 0;
	}
}

/* ================ row ops ================ */

int
editor_row_cx_to_rx(struct editor_row *row, int cx)
{
  int rx = 0;
  for (int i = 0; i < cx; i++, rx++)
    if (row->chars[i] == '\t')
      rx += (TAB_STOP_SZ - 1) - (rx % TAB_STOP_SZ);
  return rx;
}


void
editor_update_row(struct editor_row *row)
{
  /* specially render tabs */
  int tabs = 0;
  for (int i = 0; i < row->size; i++)
    if (row->chars[i] == '\t')
      tabs++;
  
  free(row->render);
  // 1 already exists for each tab
  row->render = malloc(row->size + tabs*(TAB_STOP_SZ - 1) +1);

  int idx = 0;
  for (int i = 0; i < row->size; i++)
    {
      if (row->chars[i] == '\t')
        {
          row->render[idx++] = ' ';
          while (idx % TAB_STOP_SZ != 0)
            row->render[idx++] = ' ';
        }
      else
        row->render[idx++] = row->chars[i];
    }
  
  row->render[idx] = '\0';
  row->rsize = idx;
}

void
editor_append_row(char *unterminated_s, size_t len)
{
  editor.row = realloc(editor.row,
							  sizeof *editor.row
							  * (editor.num_rows + 1));
  if (editor.row == NULL)
	die(DIE_ERROR_FMT, "realloc");
  
  int new_row = editor.num_rows;
  editor.row[new_row].size = len;
  editor.row[new_row].chars = malloc(len + 1);
  if (editor.row[new_row].chars == NULL)
	die(DIE_ERROR_FMT, "malloc");
  
  memcpy(editor.row[new_row].chars, unterminated_s, len);
  editor.row[new_row].chars[len] = '\0';

  editor.row[new_row].rsize = 0;
  editor.row[new_row].render = NULL;
  editor_update_row(&editor.row[new_row]);
  
  editor.num_rows++;
}

/* ================ file i/o ================ */

// maybe add a simple UTF-8 check ... do not support :)
// and make POSIXly
void
editor_open(char *filename)
{
  free(editor.filename);
  editor.filename = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp)
	die(DIE_ERROR_FMT, "fopen");
  
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while (
		 (linelen = getline(&line, &linecap, fp)) != -1)
	{
	  // TODO: figure out display if last line doesn't have `\n`,
      // just EOF (edge case)
	  while (linelen > 0
			 && (line[linelen - 1] == '\n' ||
				 line[linelen - 1] == '\r'))
		linelen--;
	  editor_append_row(line, linelen);
	}
  if (line == NULL)
	die(DIE_ERROR_FMT, "getline");
  free(line);
  fclose(fp);
}
	  
/* ================ input ================ */

void
editor_set_status_msg(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(editor.status_msg,
            sizeof(editor.status_msg),
            fmt,
            ap);
  va_end(ap);
  editor.status_msg_time = time(NULL);
}

void
editor_move_cursor(int c)
{
  // get the row the cursor is on
  // can be one row past the end, >= vs. ==
  struct editor_row *row =
	(editor.cy >= editor.num_rows) ?
	NULL : &editor.row[editor.cy];
	
  switch (c)
	{
	case FORWARD_CHAR:
	  // can't go past end
	  if (row && editor.cx < row->size)
		editor.cx++;
	  // at the end (or one past I guess -- to type)
	  // also not on the last line (or the one that has nothing)
	  else if (row && editor.cx  == row->size)
		{
		  editor.cy++;
		  editor.cx = 0;
		}
      else
        {
          write(STDOUT_FILENO, "\a", 1);
          editor_set_status_msg("End of buffer");
        }
	  break;	  
	case BACKWARD_CHAR:
	  if (editor.cx != 0)
		editor.cx--;
	  // beg. of a line and it's not the first, move to end of prev.
	  else if (editor.cy > 0)
		{
		  editor.cy--;
		  editor.cx = editor.row[editor.cy].size;
		}
      else
        {
          write(STDOUT_FILENO, "\a", 1);
          editor_set_status_msg("End of buffer");
        }
	  break;
	case PREV_LINE:
	  if (editor.cy != 0)	  
		editor.cy--;
      else
        {
          write(STDOUT_FILENO, "\a", 1);
          editor_set_status_msg("End of buffer");
        }
	  break;
	case NEXT_LINE:
	  // let scroll one past bottom
	  if (editor.cy < editor.num_rows)
		editor.cy++;
      else
        {
          write(STDOUT_FILENO, "\a", 1);
          editor_set_status_msg("End of buffer");
        }
	  break;
	}

  // snap back cursor if go to line with longer line of text
  row = (editor.cy >= editor.num_rows) ?
	NULL : &editor.row[editor.cy];
  int rowlen = row ? row->size : 0;
  if (editor.cx > rowlen)
	editor.cx = rowlen;
}

void
editor_process_keystroke(void)
{
  static int c;
  static int pc;

  pc = c;
  c = editor_read_key();
  
  switch (c)
	{
	case CTRL('C'):
	  if (pc == CTRL('X'))
		{
		  editor_clear_screen();
		  exit(EXIT_SUCCESS);
		}
	  break;
	case FORWARD_CHAR:
	case BACKWARD_CHAR:
	case PREV_LINE:
	case NEXT_LINE:
	  editor_move_cursor(c);
	  break;
	case SCROLL_UP:
	case SCROLL_DOWN:
	  {
        if (c == SCROLL_UP)
          editor.cy = editor.row_offset;
        else
          {
            editor.cy = editor.row_offset + editor.window_rows - 1;
            if (editor.cy > editor.num_rows)
            // one past the end, be careful with newlines at EOF              
              editor.cy = editor.num_rows;
          }
        // gain some idea of prev place        
		int iterations = editor.window_rows - 4;
		while (iterations--)
		  editor_move_cursor(c == SCROLL_UP ? PREV_LINE : NEXT_LINE);
	  }
	  break;
    case MV_BEG_OF_LINE:
      editor.cx = 0;
      break;
    case MV_END_OF_LINE:
      if (editor.cy < editor.num_rows)
        editor.cx = editor.row[editor.cy].size;
      break;
	case BEG_OF_BUF:
      editor.cx = editor.cy = editor.row_offset = 0;
	  break;
	case END_OF_BUF:
      editor.cx = 0;
      editor.cy = editor.num_rows;
	  break;
	}
}

/* ================ output ================ */

void
editor_scroll(void)
{
  // render at 0 if one past last line
  editor.rx = 0;
  if (editor.cy < editor.num_rows)
    editor.rx = editor_row_cx_to_rx(&editor.row[editor.cy],
                                    editor.cx);
  
  // above visibility
  if (editor.cy < editor.row_offset)
	editor.row_offset = editor.cy;
  // below visiblity
  else if (editor.cy
	  >= editor.row_offset + editor.window_rows)
	editor.row_offset
	  = editor.cy - editor.window_rows + 1;

  if (editor.rx < editor.col_offset)
	editor.col_offset = editor.rx;
  else if (editor.rx >= editor.col_offset + editor.window_cols)
	editor.col_offset = editor.rx - editor.window_cols + 1;
}
	  

void
editor_draw_rows(void)
{
  for (int j = 0; j < editor.window_rows; j++)
	{
	  // some rows with no content ... (past text buffer)
	  int filerow = j + editor.row_offset;
	  
	  if (filerow >= editor.num_rows)
		{
		  // no welcome message if displaying content
		  if (editor.num_rows == 0 && j == editor.window_rows / 2 - editor.window_rows / 8)
			{
			  char welcome[80];
			  int welcomelen = snprintf(welcome, sizeof(welcome),
										"Le -- version %s",
										LE_VERSION);
			  if (welcomelen > editor.window_cols)
				welcomelen = editor.window_cols;
			  int padding = (editor.window_cols - welcomelen) / 2;
			  while (padding--)
				abuf_append(" ", 1);
			  abuf_append(welcome, welcomelen);
			}
		}
	  else
		{
		  /* display starting a certain number of columns in --
             horizontal scroll */
		  int len = editor.row[filerow].rsize - editor.col_offset;
		  // maybe they're on a longer line than ours, ours goes to 0
		  if (len < 0)
			len = 0;
		  else if (len > editor.window_cols)
			len = editor.window_cols;
		  abuf_append(&editor.row[filerow].render[editor.col_offset], len);
		}
	  
      abuf_append(EOL, EOL_SZ);
	}
}

void
editor_draw_status_bar(void)
{
  abuf_append(START_INVERT_TEXT, START_INVERT_TEXT_SZ);
  char status[80];
  int len = snprintf(status, sizeof(status), " -:**-  %.20s -- line %d/%d",
                     editor.filename ? editor.filename : "*no-file*",
                     editor.cy + 1,
                     editor.num_rows
                     );
  if (len > editor.window_cols)
    len = editor.window_cols;
  abuf_append(status, len);
  for (; len < editor.window_cols; len++)
    abuf_append(" ", 1);
  abuf_append(END_INVERT_TEXT, END_INVERT_TEXT_SZ);
  abuf_append(EOL, EOL_SZ); /* make room for status msg */
}

void
editor_draw_msg_bar(void)
{
  int msg_len = strlen(editor.status_msg);
  if (msg_len > editor.window_cols)
    msg_len = editor.window_cols;
  if (msg_len && time(NULL) - editor.status_msg_time < 3)
    abuf_append(editor.status_msg, msg_len);
}
  
void
editor_refresh_screen(void)
{
  editor_scroll();

  ab.buf = NULL;
  ab.len = 0;
  
  abuf_append(HIDE_CURSOR, HIDE_CURSOR_SZ);
  abuf_append(MV_CURSOR_TOP_LEFT, MV_CURSOR_TOP_LEFT_SZ);  
  abuf_append(ERASE_DISPLAY, ERASE_DISPLAY_SZ);
  
  editor_draw_rows();
  editor_draw_status_bar();
  editor_draw_msg_bar();
  
  char buf[32];

  /* subtract off row offset to position since
  cy/rx references our position within the text file, not on the screen */
  int cursor_pos_y = editor.cy - editor.row_offset + 1;
  int cursor_pos_x = editor.rx - editor.col_offset + 1;
  snprintf(buf, sizeof(buf), MV_CURSOR_COORD_ARGS_YX, cursor_pos_y, cursor_pos_x);
  abuf_append(buf, strlen(buf));
		   
  abuf_append(UNHIDE_CURSOR, UNHIDE_CURSOR_SZ);

  write(STDOUT_FILENO, ab.buf, ab.len);
  WRITE_LOG_PTR("about to call abuf destruct in editor refresh screen", ab.buf);
  abuf_destruct();
}

/* initialization */

void
update_window_size(void)
{
  if (get_window_size(&editor.window_rows,
                      &editor.window_cols) == -1)
    die(DIE_ERROR_FMT, "get_window_size");
  /* status bar && msg bar */
  editor.window_rows -= 2;
}
                      
void handle_sigwinch(int sig [[maybe_unused]])
{
  update_window_size();
  editor_refresh_screen();
}

void
init_editor(void)
{
  /* 0-indexed within text file */
  editor.cx = editor.cy = 0;
  /* within the rendered characters */
  editor.rx = 0;

  editor.num_rows = 0;
  editor.row_offset = 0;
  editor.col_offset = 0;
  
  editor.row = NULL;
  editor.filename = NULL;
  editor.status_msg[0] = '\0';
  editor.status_msg_time = 0;

  //  editor.final_row_newline = false;
  
  update_window_size();
  signal(SIGWINCH, handle_sigwinch);
}

int
main(int argc, char *argv[])
{
  INIT_LOG("le.log");
  progname = argv[0];
  enable_raw_mode();
  init_editor();

  if (argc == 2)
	editor_open(argv[1]);
  else if (argc != 1)
    die(DIE_MSG_FMT, "bad usage");

  editor_set_status_msg("C-x C-c to quit");

  while (1)
	{
	  editor_refresh_screen();
	  editor_process_keystroke();
	}

  return 0;
}
