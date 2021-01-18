#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINECHUNK 128
#define CMDCHUNK 4
#define CMDCHUNKLEN 8
#define ENTRIESCHUNK 8
#define ENTRYTEXTCHUNK 16

const char *SAVE_FN = "todo.dat";

struct entry;

unsigned char *get_line(int *len);
unsigned char **parse_line(unsigned char *line, int len, int *n, int *args_s, int *cmd_m);
void cli(struct entry *entries, int entries_n, int entries_m);
void load_entries(struct entry *entries, FILE *fp, int *n, int *entries_m);
void save_entries(struct entry *entries, int n);
void add_entry(struct entry **entries, unsigned char *text, int *n, int *entries_m);
void remove_entry(struct entry *entries, int idx, int *n);
void display_entries(struct entry *entries, int n);
void spaced_display(struct entry *entries, int n);
void init_entries(struct entry **entries);

struct entry {
    char done;
    unsigned char *text;
};

int main() {
    FILE *save_fp;
    struct entry *entries;
    int n, entries_m;

    init_entries(&entries);

    save_fp = fopen(SAVE_FN, "rb");

    if (!save_fp) {
        save_fp = fopen(SAVE_FN, "wb");
        printf("todo.dat not found; created new\n");
        printf("tip: type \"help\"\n");
        n = 0;
        entries_m = 1;
    }
    else {
        load_entries(entries, save_fp, &n, &entries_m);
        display_entries(entries, n);
    }

    printf("\n");
    fclose(save_fp);
    cli(entries, n, entries_m);
}

unsigned char *get_line(int *len) {
    unsigned char *line, c;
    int i, j, k, size_m, leading;

    printf(">> ");

    size_m = 1;
    line = (unsigned char *)malloc(LINECHUNK);
    leading = 1;
    j = 0;

    for (i = 0; ; i++) {
        if (i % LINECHUNK == 0) {
            size_m++;
            line = (unsigned char *)realloc(line, size_m * LINECHUNK);
        }

        if ((c = getchar()) == '\n') {
            line[i] = '\0';
            break;
        }
        else if (leading) {
            if (c == ' ') {
                i--;
                continue;
            }
            else
                leading = 0;
        }

        line[i] = c;
        j++;
    }

    for (k = j - 1; k > -1; k--) {
        if (line[k] == ' ') {
            j--;
            continue;
        }
        line[k + 1] = '\0';
        break;
    }

    *len = j;
    return line;
}

unsigned char **parse_line(unsigned char *line, int len, int *cmd_n, int *args_s, int *cmd_m) {
    unsigned char **cmds;
    int i, j, k, v, w, cmdlen_m;

    cmds = (unsigned char **)malloc(CMDCHUNK * sizeof(unsigned char *));
    for (j = 0; j < CMDCHUNK; j++)
        cmds[j] = (unsigned char *)malloc(CMDCHUNKLEN);
    *cmd_n = 0;
    k = 0;
    *cmd_m = 1;
    cmdlen_m = 1;

    for (i = 0; i < len + 1; i++) {
        v = line[i] == ' ';
        w = line[i - 1] != ' ' && v;

        if (w || line[i] == '\0') {
            cmds[*cmd_n][k] = '\0';
            (*cmd_n)++;

            if (*cmd_n % CMDCHUNK == 0) {
                (*cmd_m)++;
                cmds = (unsigned char **)realloc(cmds, *cmd_m * CMDCHUNK * sizeof(unsigned char *));
                for (j = *cmd_n; j < *cmd_m * CMDCHUNK; j++)
                    cmds[j] = (unsigned char *)malloc(CMDCHUNKLEN);
            }
            
            k = 0;
            cmdlen_m = 1;
            continue;
        }
        if (v)
            continue;
        if (*cmd_n == 1 && k == 0)
            *args_s = i;

        cmds[*cmd_n][k] = line[i];
        k++;

        if (k % CMDCHUNKLEN == 0) {
            cmdlen_m++;
            cmds[*cmd_n] = (unsigned char *)realloc(cmds[*cmd_n], cmdlen_m * CMDCHUNKLEN);
        }
    }

    return cmds;
}

void cli(struct entry *entries, int entries_n, int entries_m) {
    unsigned char *line, *args, **cmds;
    const char *commands[] = {
        "quit",
        "exit",
        "save",
        "add",
        "remove",
        "do",
        "undo",
        "help"
    };
    int i, j, k, n, m, b, l, idx, len, len_cmds, exit, cmp, args_s, cmd_n, cmd_m; 
    FILE *fp;

    len_cmds = sizeof(commands) / sizeof(commands[0]);
    exit = 0;

    while (1) {
        line = get_line(&len);

        if (len == 0)
            continue;

        cmds = parse_line(line, len, &cmd_n, &args_s, &cmd_m);
        
        if (cmd_n == 0)
            continue;

        for (i = 0; i < len_cmds; i++) {
            cmp = strcmp(cmds[0], commands[i]);
            if (cmp == 0) {
                switch (i) {
                case 0:
                case 1:
                    exit = 1;
                    break;
                case 2:
                    if (entries_n > 0) {
                        save_entries(entries, entries_n);
                        printf("saved %d entries\n", entries_n);
                    }
                    else {
                        fp = fopen(SAVE_FN, "wb");
                        fclose(fp);
                        printf("no entries to save; cleared todo.dat\n");
                    }
                    break;
                case 3:
                    if (cmd_n > 1) {
                        m = (len - args_s) + 1;
                        args = (unsigned char *)malloc(m);
                        n = args_s;
                        for (j = 0; j < m; j++) {
                            args[j] = line[n];
                            n++;
                        }
                        args[m - 1] = '\0';
                        add_entry(&entries, args, &entries_n, &entries_m);
                        spaced_display(entries, entries_n);
                    }
                    break;
                case 4:
                    if (cmd_n > 1) {
                        idx = atoi(cmds[1]);
                        if (idx > 0 && idx - 1 < entries_n) {
                            remove_entry(entries, idx - 1, &entries_n);
                            spaced_display(entries, entries_n);
                        }
                    }
                    break;
                case 5:
                case 6:
                    if (cmd_n > 1) {
                        idx = atoi(cmds[1]);
                        if (idx > 0 && idx - 1 < entries_n) {
                            entries[idx - 1].done = i == 5 ? 1 : 0;
                            spaced_display(entries, entries_n);
                        }
                    }
                    break;
                case 7:
                    printf("A minimal command-line TODO list written by Cocoox\n\n");
                    printf("quit/exit      -  close the program (DOES NOT SAVE CHANGES)\n");
                    printf("save           -  write changes to todo.dat\n");
                    printf("add <str>      -  create a new entry named <str>\n");
                    printf("remove <int>   -  remove the entry at position <int>\n");
                    printf("do/undo <int>  -  check/uncheck the entry at position <int>\n\n");
                    break;
                default:
                    break;
                }
            }
            if (exit)
                break;
        }

        for (k = 0; k < cmd_m * CMDCHUNK; k++)
            free(cmds[k]);

        free(cmds);
        free(line);

        if (exit)
            break;
    }
}

void load_entries(struct entry *entries, FILE *fp, int *n, int *entries_m) {
    unsigned char b;
    int i, j, c, f, s, idx, text_m;

    fseek(fp, 0, SEEK_END);
    s = ftell(fp);
    if (s < 2) {
        *n = 0;
        return;
    }
    fseek(fp, 0, SEEK_SET);

    idx = 0;
    *n = 0;
    f = 1;
    text_m = 1;
    *entries_m = 1;

    while ((c = getc(fp)) != EOF) {
        b = c;

        if (f) {
            entries[*n].done = b;
            f = 0;
        }
        else if (b) {
            entries[*n].text[idx] = b;
            idx++;

            if (idx % ENTRYTEXTCHUNK == 0) {
                text_m++;
                entries[*n].text = (unsigned char *)realloc(entries[*n].text, text_m * ENTRYTEXTCHUNK + 1);
            }
        }
        else {
            entries[*n].text[idx] = '\0';
            (*n)++;

            if (*n % ENTRIESCHUNK == 0) {
                (*entries_m)++;
                entries = (struct entry *)realloc(entries, *entries_m * ENTRIESCHUNK * sizeof(struct entry));
                for (j = *n; j < *entries_m * CMDCHUNK; j++)
                    entries[j].text = (unsigned char *)malloc(ENTRYTEXTCHUNK + 1);
            }

            f = 1;
            idx = 0;
        }
    }
}

void save_entries(struct entry *entries, int n) {
    FILE *save_fp;
    int i;
    unsigned char *text;
    unsigned char z;

    save_fp = fopen(SAVE_FN, "wb");
    z = 0;

    for (i = 0; i < n; i++) {
        fwrite(&entries[i].done, 1, 1, save_fp);
        text = entries[i].text;
        fwrite(text, 1, strlen(text), save_fp);
        fwrite(&z, 1, 1, save_fp);
    }

    fclose(save_fp);
}

void add_entry(struct entry **entries, unsigned char *text, int *n, int *entries_m) {
    int f;
    struct entry e;

    f = ENTRIESCHUNK * (*entries_m) - *n;
    if (!f) {
        (*entries_m)++;
        *entries = (struct entry *)realloc(*entries, *entries_m * ENTRIESCHUNK * sizeof(struct entry));
    }

    e = (struct entry) { .done = 0, .text = text};
    (*entries)[*n] = e;
    (*n)++;
}

void remove_entry(struct entry *entries, int idx, int *n) {
    int i;

    if (idx + 1 == *n)
        (*n)--;
    else {
        for (i = idx; i < *n - 1; i++)
            entries[i] = entries[i + 1];
        (*n)--;
    }
}

void display_entries(struct entry *entries, int n) {
    int i;
    struct entry entry;

    for (i = 0; i < n; i++) {
        entry = entries[i];
        printf("%d [%c] ", i + 1, entry.done ? 'x' : ' ');
        printf(entry.text);
        printf("\n");
    }
}

void spaced_display(struct entry *entries, int n) {
    printf("\n");
    display_entries(entries, n);
    printf("\n");
}

void init_entries(struct entry **entries) {
    int i;

    *entries = (struct entry *)malloc(sizeof(struct entry) * ENTRIESCHUNK);
    for (i = 0; i < ENTRIESCHUNK; i++)
        (*entries[i]).text = (unsigned char *)malloc(ENTRYTEXTCHUNK + 1);
}
