/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
        Coded by Charles Rector AKA aen
        last updated: late dec '99
*/

// V2 SCRIPT EDITOR
// /////////////////////////////////////////////////////////////////////////////////////

static linked_list *lines = 0;
static linked_node *current_line = 0;
static int current_line_n = 0;
static linked_node *top = 0;
static int top_n = 0;
static int cursor_timer = 0;
static int charx = 0;
static int leftx = 0;
static string_k v2se_filename = "";
static byte *backdrop = 0;
static int backdrop_width = 0;
static int backdrop_length = 0;

static int V2SE_Rows() {
    return (gfx.YRes() - (Font_GetLength(0) + 2) - 2) / Font_GetLength(0);
}

static int V2SE_Columns() { return (gfx.XRes() - 2) / Font_GetWidth(0); }

static int V2SE_LoadFile(string_k filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        return 0;
    }

    v2se_filename = filename;

    if (lines) delete lines;
    lines = new linked_list;

    char text[1024 + 1] = {0};
    while (fgets(text, 1024, fp)) {
        string_k s = "";
        int seek = 0, z = strlen(text);
        while (seek < z && text[seek] != '\r' && text[seek] != '\n') {
            if ('\t' == text[seek])
                s += "    ";
            else
                s += text[seek];
            seek++;
        }
        lines->insert_tail((linked_node *)new string_k(s));

        // arbitrary limit of 4k lines for now
        if (lines->number_nodes() >= 1024 * 4) break;
    }

    fclose(fp);

    return 1;
}

static int V2SE_Init() {
    if (!V2SE_LoadFile(Con_GetArg(1))) return 0;

    backdrop = 0;
    /*	if (!backdrop)
                    backdrop=Image_LoadBuf("vcedit.pcx");
            if (backdrop)
            {
                    backdrop_width	=Image_Width();
                    backdrop_length	=Image_Length();
            }*/

    // start at the top
    top = current_line = lines->head();
    top_n = current_line_n = 1;

    cursor_timer = 0;
    leftx = charx = 0;

    return 1;
}

static void V2SE_Shutdown() {
    if (lines) {
        // destroy lines
        lines->go_head();
        while (lines->head()) {
            string_k *s = (string_k *)lines->head();
            lines->unlink((linked_node *)s);
            delete s;
        }
    }
    // destroy linklist object
    delete lines;
    lines = 0;

    if (backdrop) vfree(backdrop);
    backdrop = 0;

    current_line = 0;
    top = 0;

    current_line_n = 0;
    top_n = 0;

    v2se_filename = "";
}

static void V2SE_Paint() {
    if (backdrop) {
        /*		LFB_BlitWrap(
			leftx*Font_GetWidth(0), (top_n*Font_GetLength(0)),
			backdrop_width, backdrop_length,
			backdrop, 0, 0);*/  // TODO: put this back
    } else {
        gfx.RectFill(0, 0, gfx.XRes() - 1, gfx.YRes() - 1, 0, 0);
    }
    if (!lines->number_nodes()) return;

    int x, y;
    linked_node *iter;

    x = y = 1;

    gfx.RectFill(0, 0, gfx.XRes(), Font_GetLength(0) + 1, 144, 0);

    Font_GotoXY(x, y);
    Font_Print(0, va(" %-12s", v2se_filename));
    Font_GotoXY(gfx.XRes() - 1 - Font_GetWidth(0) * 19, y);
    Font_Print(0, va("line %4d row %4d", current_line_n, charx + 1));
    y += Font_GetLength(0) + 1;

    iter = top;
    int row = V2SE_Rows();
    do {
        // highlight current line
        if (iter == current_line) {
            gfx.RectFill(0, y, gfx.XRes(), y + Font_GetLength(0) - 1, 32, 0);
        }

        Font_GotoXY(x, y);
        // clipping rawks. ^_^
        Font_Print(0, ((string_k *)iter)->mid(leftx, V2SE_Columns()).c_str());

        y += Font_GetLength(0);

        iter = iter->next();

    } while (
        --row && y + Font_GetLength(0) < gfx.YRes() && iter != lines->head());

    // draw cursor?
    if (cursor_timer < 20) {
        Font_GotoXY(1 + (Font_GetWidth(0) * (charx - leftx)),
            Font_GetLength(0) + 2 +
                (current_line_n - top_n) * Font_GetLength(0) + 1);

        Font_Print(0, "-");
    }
}

static int V2SE_MoveUp() {
    if (current_line->prev() != lines->head()->prev()) {
        current_line = current_line->prev();
        current_line_n--;
        if (current_line_n < top_n) {
            top = top->prev();
            top_n--;
        }

        return 0;
    }
    return 1;
}

static int V2SE_MoveUpCount(int n) {
    if (n < 1) return 0;

    int hit = 0;
    while (n--) hit += V2SE_MoveUp();
    return hit;
}

static int V2SE_MoveDown() {
    if (current_line->next() != lines->head()) {
        current_line = current_line->next();
        current_line_n++;
        if (current_line_n >= top_n + V2SE_Rows()) {
            top = top->next();
            top_n++;
        }

        return 0;
    }
    return 1;
}

static int V2SE_MoveDownCount(int n) {
    if (n < 1) return 0;

    int hit = 0;
    while (n--) hit += V2SE_MoveDown();
    return hit;
}

static int V2SE_MoveLeft() {
    if (charx > 0) {
        charx--;

        if (charx < leftx) {
            leftx--;
        }
        return 0;
    }
    return 1;
}

static int V2SE_MoveLeftCount(int n) {
    if (n < 1) return 0;

    int hit = 0;
    while (n--) hit += V2SE_MoveLeft();
    return hit;
}

static int V2SE_MoveRight() {
    if (charx + 1 < 1024) {
        charx++;

        if (charx > leftx + V2SE_Columns()) {
            leftx++;
        }
        return 0;
    }
    return 1;
}

int V2SE_MoveRightCount(int n) {
    if (n < 1) return 0;

    int hit = 0;
    while (n--) hit += V2SE_MoveRight();
    return hit;
}

static int V2SE_InsertString(string_k add) {
    string_k *s = (string_k *)current_line;

    if (s->length() + add.length() > 1024) return 0;

    if (charx >= s->length()) {
        int x = charx - s->length();

        if (s->length() + x + add.length() > 1024) return 0;

        while (x--) {
            (*s) += ' ';
        }
        (*s) += add;
    } else {
        (*s) = s->mid(0, charx) + add + s->right(s->length() - charx);
    }

    int x = add.length();
    while (x--) {
        V2SE_MoveRight();
    }

    return add.length();
}

static void V2SE_InsertKey(int ch) {
    // printable?
    if (ch > 31 && ch < 128) {
        V2SE_InsertString((char)ch);
    }
}

static void V2SE_Home() { charx = leftx = 0; }

static void V2SE_End() {
    charx = ((string_k *)current_line)->length();

    leftx = charx - V2SE_Columns() + 1;
    if (leftx < 0) leftx = 0;
}

static int V2SE_PageUp() { return V2SE_MoveUpCount(V2SE_Rows() - 1); }

static int V2SE_PageDown() { return V2SE_MoveDownCount(V2SE_Rows() - 1); }

static void V2SE_Tab() {
    int n = 4 - (charx % 4);
    while (n--) V2SE_InsertKey(' ');
}

static void V2SE_Enter() {
    string_k *s = (string_k *)current_line;
    string_k add = "";
    if (charx < s->length()) {
        add = s->mid(charx, 1024);
        (*s) = s->left(charx);
    }
    lines->set_current(current_line);
    lines->insert_after_current((linked_node *)new string_k(add));

    V2SE_MoveDown();
    V2SE_Home();
}

static void V2SE_Delete() {
    string_k *s = (string_k *)current_line;

    if (charx < s->length()) {
        (*s) = s->mid(0, charx) + s->right(s->length() - (charx + 1));
    } else {
        if (current_line->next() != lines->head()) {
            string_k *r = (string_k *)current_line->next();
            V2SE_MoveLeftCount(V2SE_InsertString(*r));
            lines->unlink((linked_node *)r);
            delete r;
        }
    }
}

static void V2SE_Backspace() {
    if (V2SE_MoveLeft()) {
        V2SE_MoveUp();
        V2SE_End();
    }
    V2SE_Delete();
}

static void V2SE_Save() {
    FILE *fp = fopen("crap.dat", "wt");

    lines->go_head();
    do {
        string_k s = *(string_k *)lines->current();
        s += '\n';

        fputs(s.c_str(), fp);

        lines->go_next();
    } while (lines->current() != lines->head());

    fclose(fp);
}

static int editor_processing = 0;

void V2SE_Key(int key) {
    switch (key) {
    case DIK_ESC:
        editor_processing = 0;
        break;

    case DIK_UP:
        V2SE_MoveUp();
        break;
    case DIK_DOWN:
        V2SE_MoveDown();
        break;
    case DIK_LEFT:
        V2SE_MoveLeft();
        break;
    case DIK_RIGHT:
        V2SE_MoveRight();
        break;

    case DIK_HOME:
        V2SE_Home();
        break;
    case DIK_END:
        V2SE_End();
        break;

    case DIK_PGUP:
        V2SE_PageUp();
        break;
    case DIK_PGDN:
        V2SE_PageDown();
        break;

    case DIK_TAB:
        V2SE_Tab();
        break;
    case DIK_ENTER:
        V2SE_Enter();
        break;

    //      case DIK_DEL:     V2SE_Delete();          break;
    case DIK_BACK:
        V2SE_Backspace();
        break;

    case DIK_F12:
        V2SE_Save();
        break;

    default:
        V2SE_InsertKey(input.Scan2ASCII(key));
        break;
    }
}

static void V2SE_Main() {
    if (!V2SE_Init()) {
        Console_Printf(va("Unable to load script %s.", Con_GetArg(1)));
        return;
    }
    LogDone();

    cursor_timer = 0;
    editor_processing = 1;
    while (editor_processing) {
        int tag = systemtime;

        V2SE_Paint();
        gfx.ShowPage();

        //		input.Update();
        while (int c = input.GetKey()) V2SE_Key(c);

        CheckMessages();

        tag = systemtime - tag;
        while (tag) {
            tag--;
            cursor_timer++;
            if (cursor_timer >= 40) cursor_timer -= 40;
        }
    }

    V2SE_Shutdown();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
