/* Copyright (c) 2023, Arteen Abrishami. All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed by Arteen Abrishami.
 * Neither the name of Arteen Abrishami nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY ARTEEN ABRISHAMI AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*  ================ INCLUDES  ================ */

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

/* TODO:
 * catch SIGWINCH for window size
 * https://unix.stackexchange.com/questions/580362/how-are-terminal-information-such-as-window-size-sent-to-a-linux-program
 */
 
/*  ================ DEFINES  ================ */

#define LE_VERSION "0.0.1"

/*  ================ screen management  ================ */

/* https://stackoverflow.com/questions/39188508/how-curses-preserves-screen-contents */
#define ENABLE_ALT_SCREEN "\x1b[?1049h"
#define DISABLE_ALT_SCREEN "\x1b[?1049l"
#define ENABLE_ALT_SCREEN_SZ 8
#define DISABLE_ALT_SCREEN_SZ 8
/* from the obscure depths of the internet
 * https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking */
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

/* ================ LOG (optional) ================ */

// #define LOG

#ifdef LOG

#include <fcntl.h>

#define INIT_LOG(f) do {                                    \
	int fd = open(f, O_CREAT | O_WRONLY | O_TRUNC);         \
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

#define WRITE_LOG_CHAR(s, c) do {									\
  if (iscntrl(c))													\
	fprintf(stderr, "char: %d ('^%c')\r\n", c, c + 'A' - 1);		\
  else																\
	fprintf(stderr, "char: %d ('%c')\r\n", c, c);					\
  } while (0)

#else

#define INIT_LOG(f)
#define DELIM_WRITE_LOG(s)
#define WRITE_LOG(s)
#define WRITE_LOG_STR(s, l)
#define WRITE_LOG_INT(s, i)
#define WRITE_LOG_CHAR(s, c)

#endif

/* ================ GLOBAL STRUCTS ================ */

struct editor_row
{
  /* size of actual chars present in row */
  int size;
  /* size of the rendered chars on screen */
  int rsize;
  char *chars;
  char *render;
};

struct editor_state_struct
{
  struct termios orig_termios;
  struct editor_row *row;
  char *filename;
  char status_msg[80];
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
     adjusted for tabs and special chars, adjust for real cursor pos */
  int rx;
  /* for the terminal window */
  int window_rows, window_cols;
} editor;

struct abuf
{
  int len;
  char *buf;
};

/* append buffer */

void
abuf_append(struct abuf *ab, const char *s, int len)
{
  char *new_buf = realloc(ab->buf, ab->len + len);
  if (new_buf == NULL)
	return;
  memcpy(&new_buf[ab->len], s, len);
  ab->buf = new_buf;
  ab->len += len;
}

void
abuf_destruct(struct abuf *ab)
{
  free(ab->buf);
}

/* misc */

inline void
editor_clear_screen(void);

void die(const char *s)
{
  editor_clear_screen();
  perror(s);
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
	die("write");
}

/* terminal control */

void
disable_raw_mode(void)
{
  write(STDOUT_FILENO, DISABLE_ALT_SCREEN DISABLE_MOUSE_TRACKING,
      DISABLE_ALT_SCREEN_SZ + DISABLE_MOUSE_TRACKING_SZ);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.orig_termios) == -1)
	die("tcsetattr");
}

void
enable_raw_mode(void)
{
  write(STDOUT_FILENO, ENABLE_ALT_SCREEN ENABLE_MOUSE_TRACKING,
        ENABLE_ALT_SCREEN_SZ + ENABLE_MOUSE_TRACKING_SZ);
  if (tcgetattr(STDIN_FILENO, &editor.orig_termios) == -1)
	die("tcgetattr");
  atexit(disable_raw_mode);

  /* https://man7.org/linux/man-pages/man3/termios.3.html */
  struct termios raw = editor.orig_termios;
  raw.c_iflag &= ~(BRKINT | INPCK | PARMRK | INLCR | IGNCR | ISTRIP | ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cflag &= ~(CSIZE | PARENB);
  raw.c_cflag |= (CS8);


  // test getting rid of this ...
  // don't block longer than 1/10 sec for reads
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	die("tcgetattr");
}

int
editor_read_key(void)
{
  int nread;
  char c;
  
  while ( (nread = read(STDIN_FILENO, &c, 1)) != 1 )
	if (nread == -1)
	  die("read");
  
  if (c == '\x1b')
	{
	  char seq[3];

	  // escape or meta (read timed out)
	  // always <esc>[<something> unless one of our own ...
	  if (read(STDIN_FILENO, &seq[0], 1) != 1)
		return '\x1b';

	  // quick fix for M-v
	  if (seq[0] == 'v')
		return SCROLL_UP;
	  
	  if (read(STDIN_FILENO, &seq[1], 1) != 1)
		return '\x1b';

	  if (seq[0] == '[')
		{
		  if (seq[1] >= '0' && seq[1] <= '9')
			{
			  if (read(STDIN_FILENO, &seq[2], 1) != 1)
				return '\x1b';
			  if (seq[2] == '~')
				switch (seq[1])
				  {
					// page up and down keys
					// caught on MacOS Terminal.app (fn+<keyup/down>)
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
			switch (seq[1])
			  {
				// arrow keys
			  case 'A':
				return PREV_LINE;
			  case 'B':
				return NEXT_LINE;
			  case 'C':
				return FORWARD_CHAR;
			  case 'D':
				return BACKWARD_CHAR;
                // possible page up and down keys
			  case 'H':
				return BEG_OF_BUF;
			  case 'F':
				return END_OF_BUF;
              case 'M':
                {
                  // scrolling
                  char scroll[3];
                  if (read(STDIN_FILENO, scroll, 3) != 3)
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
		switch (seq[1])
		  {
		  case 'H':
			return BEG_OF_BUF;
		  case 'F':
			return END_OF_BUF;
		  }
	  
	  // escape or meta
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
	die("read");

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
	  // backup method
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

/* row ops */

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
  // specially render tabs
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
	die("realloc");
  
  int new_row = editor.num_rows;
  editor.row[new_row].size = len;
  editor.row[new_row].chars = malloc(len + 1);
  if (editor.row[new_row].chars == NULL)
	die("malloc");
  
  memcpy(editor.row[new_row].chars, unterminated_s, len);
  editor.row[new_row].chars[len] = '\0';

  editor.row[new_row].rsize = 0;
  editor.row[new_row].render = NULL;
  editor_update_row(&editor.row[new_row]);
  
  editor.num_rows++;
}

/* file i/o */

// maybe add a simple UTF-8 check ... do not support :)
// and make POSIXly
void
editor_open(char *filename)
{
  free(editor.filename);
  editor.filename = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp)
	die("fopen");
  
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while (
		 (linelen = getline(&line, &linecap, fp)) != -1)
	{
	  // TODO: figure out display if last line doesn't have `\n`, just EOF (edge case)
	  while (linelen > 0
			 && (line[linelen - 1] == '\n' ||
				 line[linelen - 1] == '\r'))
		linelen--;
	  editor_append_row(line, linelen);
	}
  if (line == NULL)
	die("getline");
  free(line);
  fclose(fp);
}
	  
/* input */

void
editor_set_status_msg(const char *, ...);

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
              editor.cy = editor.num_rows; // one past the end, be careful with newlines at EOF
          }
		int iterations = editor.window_rows - 4; // gain some idea of prev place
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

/* output */

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
editor_draw_rows(struct abuf *ab)
{
  for (int j = 0; j < editor.window_rows; j++)
	{
	  // some rows with no content ... (past text buffer)
	  int filerow = j + editor.row_offset;
	  
	  if (filerow >= editor.num_rows)
		{
		  // no welcome message if displaying content
		  if (editor.num_rows == 0 && j == editor.window_rows / 3)
			{
			  char welcome[80];
			  int welcomelen = snprintf(welcome, sizeof(welcome),
										"Le -- version %s",
										LE_VERSION);
			  if (welcomelen > editor.window_cols)
				welcomelen = editor.window_cols;
			  int padding = (editor.window_cols - welcomelen) / 2;
			  if (padding)
				{
				  abuf_append(ab, "~", 1);
				  padding--;
				}
			  while (padding--)
				abuf_append(ab, " ", 1);
			  abuf_append(ab, welcome, welcomelen);
			}
		  else
			abuf_append(ab, "~", 1);
		}
	  else
		{
		  // display starting a certain number of columns in -- horizontal scroll
		  int len = editor.row[filerow].rsize - editor.col_offset;
		  // maybe they're on a longer line than ours, ours goes to 0
		  if (len < 0)
			len = 0;
		  else if (len > editor.window_cols)
			len = editor.window_cols;
		  abuf_append(ab, &editor.row[filerow].render[editor.col_offset], len);
		}
	  
      abuf_append(ab, EOL, EOL_SZ);
	}
}

void
editor_draw_status_bar(struct abuf *ab)
{
  abuf_append(ab, START_INVERT_TEXT, START_INVERT_TEXT_SZ);
  char status[80];
  int len = snprintf(status, sizeof(status), " -:**-  %.20s -- line %d/%d",
                     editor.filename ? editor.filename : "*no-file*",
                     editor.cy + 1,
                     editor.num_rows
                     );
  if (len > editor.window_cols)
    len = editor.window_cols;
  abuf_append(ab, status, len);
  for (; len < editor.window_cols; len++)
    abuf_append(ab, " ", 1);
  abuf_append(ab, END_INVERT_TEXT, END_INVERT_TEXT_SZ);
  abuf_append(ab, EOL, EOL_SZ); /* make room for status msg */
}

void
editor_draw_msg_bar(struct abuf *ab)
{
  int msg_len = strlen(editor.status_msg);
  if (msg_len > editor.window_cols)
    msg_len = editor.window_cols;
  if (msg_len && time(NULL) - editor.status_msg_time < 3)
    abuf_append(ab, editor.status_msg, msg_len);
}
  
void
editor_refresh_screen(void)
{
  editor_scroll();
  
  struct abuf ab = ABUF_INIT;

  abuf_append(&ab, HIDE_CURSOR, HIDE_CURSOR_SZ);
  abuf_append(&ab, MV_CURSOR_TOP_LEFT, MV_CURSOR_TOP_LEFT_SZ);  
  abuf_append(&ab, ERASE_DISPLAY, ERASE_DISPLAY_SZ);
  
  editor_draw_rows(&ab);
  editor_draw_status_bar(&ab);
  editor_draw_msg_bar(&ab);
  
  char buf[32];

  // subtract off row offset to position, since cy/rx references our position within the text file, not on the screen
  int cursor_pos_y = editor.cy - editor.row_offset + 1;
  int cursor_pos_x = editor.rx - editor.col_offset + 1;
  snprintf(buf, sizeof(buf), MV_CURSOR_COORD_ARGS_YX, cursor_pos_y, cursor_pos_x);
  abuf_append(&ab, buf, strlen(buf));
		   
  abuf_append(&ab, UNHIDE_CURSOR, UNHIDE_CURSOR_SZ);

  write(STDOUT_FILENO, ab.buf, ab.len);
  abuf_destruct(&ab);
}

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

/* initialization */

void
init_editor(void)
{
  /* compiler can optimize away possibly,
     since global struct 0-initialized */
  /* terminal cursor pos starts at 1 index btw */
  editor.cx = editor.cy = 0;
  editor.rx = 0;
  
  editor.num_rows = 0;
  editor.row_offset = 0;
  editor.col_offset = 0;
  
  editor.row = NULL;
  editor.filename = NULL;
  editor.status_msg[0] = '\0';
  editor.status_msg_time = 0;
  
  if (get_window_size(&editor.window_rows,
					  &editor.window_cols) == -1)
	die("get_window_size");

  /* make room for status msg and bar */
  editor.window_rows -= 2;
}	  

int
main(int argc, char *argv[])
{
  INIT_LOG("le.log");
  // add tty check
  enable_raw_mode();
  init_editor();

  if (argc >= 2)
	editor_open(argv[1]);

  WRITE_LOG_INT("Window rows", editor.window_rows);
  WRITE_LOG_INT("Num row buffers", (int) editor.num_rows);

  editor_set_status_msg("C-x C-c to quit");

  while (1)
	{
	  editor_refresh_screen();
	  WRITE_LOG_INT("editor.cy", editor.cy);
	  WRITE_LOG_INT("editor.row_offset", (int) editor.row_offset);
	  editor_process_keystroke();
	}

  return 0;
}
