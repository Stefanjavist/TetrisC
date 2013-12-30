typedef unsigned char      u8;
typedef signed   char      s8;
typedef unsigned short     u16;
typedef signed   short     s16;
typedef unsigned int       u32;
typedef signed   int       s32;
typedef unsigned long long u64;
typedef signed   long long s64;

#define noreturn __attribute__((noreturn)) void

typedef enum bool {
    false,
    true
} bool;

/* Port I/O */

static inline u8 inb(u16 p)
{
    u8 r;
    asm("inb %1, %0" : "=a" (r) : "dN" (p));
    return r;
}

static inline void outb(u16 p, u8 d)
{
    asm("outb %1, %0" : : "dN" (p), "a" (d));
}

noreturn reset(void)
{
    u8 x;
    do {
        x = inb(0x64);
    } while (x & 0x02);
    outb(0x64, 0xFE);
    while (true)
        asm("hlt");
}

/* Timing */

static inline u64 rdtsc(void)
{
    u32 hi, lo;
    asm("rdtsc" : "=a" (lo), "=d" (hi));
    return ((u64) lo) | (((u64) hi) << 32);
}

u8 rtcs(void)
{
    u8 last = 0, sec;
    do { /* until value is the same twice in a row */
        /* wait for update not in progress */
        do { outb(0x70, 0x0A); } while (inb(0x71) & 0x80);
        outb(0x70, 0x00);
        sec = inb(0x71);
    } while (sec != last && (last = sec));
    return sec;
}

u64 tps(void)
{
    static u64 ti = 0, dt = 0;
    static u8 last_sec = 0xFF;
    u8 sec = rtcs();
    if (sec != last_sec) {
        last_sec = sec;
        u64 tf = rdtsc();
        dt = tf - ti;
        ti = tf;
    }
    return dt;
}

enum timer {
    TIMER_UPDATE,
    TIMER__LENGTH
};

u64 timers[TIMER__LENGTH] = {0};

bool interval(enum timer timer, u64 ticks)
{
    u64 tf = rdtsc();
    if (tf - timers[timer] >= ticks) {
        timers[timer] = tf;
        return true;
    } else return false;
}

bool wait(enum timer timer, u64 ticks)
{
    if (timers[timer]) {
        if (rdtsc() - timers[timer] >= ticks) {
            timers[timer] = 0;
            return true;
        } else return false;
    } else {
        timers[timer] = rdtsc();
        return false;
    }
}

/* Video Output */

enum color {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    GRAY,
    BRIGHT
};

#define COLS (80)
#define ROWS (25)
u16 *const video = (u16*) 0xB8000;

void putc(u8 x, u8 y, enum color fg, enum color bg, char c)
{
    u16 z = (bg << 12) | (fg << 8) | c;
    video[y * COLS + x] = z;
}

void puts(u8 x, u8 y, enum color fg, enum color bg, const char *s)
{
    for (; *s; s++, x++)
        putc(x, y, fg, bg, *s);
}

void clear(enum color bg)
{
    u8 x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

/* Keyboard Input */

#define KEY_D     (0x20)
#define KEY_R     (0x13)
#define KEY_UP    (0x48)
#define KEY_DOWN  (0x50)
#define KEY_LEFT  (0x4B)
#define KEY_RIGHT (0x4D)
#define KEY_ENTER (0x1C)
#define KEY_SPACE (0x39)

u8 scan(void)
{
    static u8 key = 0;
    u8 scan = inb(0x60);
    if (scan != key)
        return key = scan;
    else return 0;
}

/* PC Speaker */

void pcspk_freq(u32 freq)
{
    u32 div = 1193180 / freq;
    outb(0x43, 0xB6);
    outb(0x42, (u8) div);
    outb(0x42, (u8) (div >> 8));
}

void pcspk_on(void)
{
    outb(0x61, inb(0x61) | 0x3);
}

void pcspk_off(void)
{
    outb(0x61, inb(0x61) & 0xFC);
}

/* Formatting */

char *itoa(u32 n, u8 r, u8 w)
{
    static const char d[16] = "0123456789ABCDEF";
    static char s[34];
    s[33] = 0;
    u8 i = 33;
    do {
        i--;
        s[i] = d[n % r];
        n /= r;
    } while (i > 33 - w);
    return (char *) (s + i);
}

/* Random */

u32 rand(u32 range)
{
    return (u32) rdtsc() % range;
}

/* Tetris */

u8 TETRIS[7][4][4][4] = {
    { /* I */
        {{0,0,0,0},
         {4,4,4,4},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,4,0,0},
         {0,4,0,0},
         {0,4,0,0},
         {0,4,0,0}},
        {{0,0,0,0},
         {4,4,4,4},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,4,0,0},
         {0,4,0,0},
         {0,4,0,0},
         {0,4,0,0}}
    },
    { /* J */
        {{0,0,0,0},
         {7,7,7,0},
         {0,0,7,0},
         {0,0,0,0}},
        {{0,7,0,0},
         {0,7,0,0},
         {7,7,0,0},
         {0,0,0,0}},
        {{7,0,0,0},
         {7,7,7,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,7,7,0},
         {0,7,0,0},
         {0,7,0,0},
         {0,0,0,0}}
    },
    { /* L */
        {{0,0,0,0},
         {5,5,5,0},
         {5,0,0,0},
         {0,0,0,0}},
        {{5,5,0,0},
         {0,5,0,0},
         {0,5,0,0},
         {0,0,0,0}},
        {{0,0,5,0},
         {5,5,5,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,5,0,0},
         {0,5,0,0},
         {0,5,5,0},
         {0,0,0,0}}
    },
    { /* O */
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}}
    },
    { /* S */
        {{0,0,0,0},
         {0,2,2,0},
         {2,2,0,0},
         {0,0,0,0}},
        {{0,2,0,0},
         {0,2,2,0},
         {0,0,2,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,2,2,0},
         {2,2,0,0},
         {0,0,0,0}},
        {{0,2,0,0},
         {0,2,2,0},
         {0,0,2,0},
         {0,0,0,0}}
    },
    { /* T */
        {{0,0,0,0},
         {6,6,6,0},
         {0,6,0,0},
         {0,0,0,0}},
        {{0,6,0,0},
         {6,6,0,0},
         {0,6,0,0},
         {0,0,0,0}},
        {{0,6,0,0},
         {6,6,6,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,6,0,0},
         {0,6,6,0},
         {0,6,0,0},
         {0,0,0,0}}
    },
    { /* Z */
        {{0,0,0,0},
         {3,3,0,0},
         {0,3,3,0},
         {0,0,0,0}},
        {{0,0,3,0},
         {0,3,3,0},
         {0,3,0,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {3,3,0,0},
         {0,3,3,0},
         {0,0,0,0}},
        {{0,0,3,0},
         {0,3,3,0},
         {0,3,0,0},
         {0,0,0,0}}
    }
};

#define WELL_WIDTH (10)
#define WELL_HEIGHT (22)
#define WELL_X (COLS / 2 - WELL_WIDTH)
u8 well[WELL_HEIGHT][WELL_WIDTH];

struct {
    u8 i, r;
    s8 x, y, g;
} current;

bool collide(u8 i, u8 r, s8 x, s8 y)
{
    u8 xx, yy;
    for (yy = 0; yy < 4; yy++)
        for (xx = 0; xx < 4; xx++)
            if (TETRIS[i][r][yy][xx])
                if (x + xx < 0 || x + xx >= WELL_WIDTH ||
                    y + yy < 0 || y + yy >= WELL_HEIGHT ||
                    well[y + yy][x + xx])
                        return true;
    return false;
}

void spawn(void)
{
    current.i = rand(7);
    current.r = 0;
    current.x = WELL_WIDTH / 2 - 2;
    current.y = 0;
}

void ghost(void)
{
    s8 y;
    for (y = current.y; y < WELL_HEIGHT; y++)
        if (collide(current.i, current.r, current.x, y))
            break;
    current.g = y - 1;
}

bool move(s8 dx, s8 dy)
{
    if (collide(current.i, current.r, current.x + dx, current.y + dy))
        return false;
    current.x += dx;
    current.y += dy;
    return true;
}

void rotate(void)
{
    u8 r = (current.r + 1) % 4;
    if (collide(current.i, r, current.x, current.y))
        return;
    current.r = r;
}

void lock(void)
{
    u8 x, y;
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[current.i][current.r][y][x])
                well[current.y + y][current.x + x] =
                    TETRIS[current.i][current.r][y][x];
}

void update(void)
{
    /* Gravity */
    if (!move(0, 1)) {
        lock();
        spawn();
    }

    /* Row clearing */
    u8 x, y, a, yy;
    for (y = 0; y < WELL_HEIGHT; y++) {
        for (a = 0, x = 0; x < WELL_WIDTH; x++)
            if (well[y][x])
                a++;
        if (a != WELL_WIDTH)
            continue;

        for (yy = y; yy > 0; yy--)
            for (x = 0; x < WELL_WIDTH; x++)
                well[yy][x] = well[yy - 1][x];
    }
}

void drop(void)
{
    current.y = current.g;
    update();
}

void draw(void)
{
    u8 x, y;

    /* Border */
    for (y = 2; y < WELL_HEIGHT; y++) {
        putc(WELL_X - 1,            y, BLACK, GRAY, ' ');
        putc(COLS / 2 + WELL_WIDTH, y, BLACK, GRAY, ' ');
    }
    for (x = 0; x < WELL_WIDTH * 2 + 2; x++)
        putc(WELL_X + x - 1, WELL_HEIGHT, BLACK, GRAY, ' ');

    /* Well */
    for (y = 0; y < 2; y++)
        for (x = 0; x < WELL_WIDTH; x++)
            puts(WELL_X + x * 2, y, BLACK, BLACK, "  ");
    for (y = 2; y < WELL_HEIGHT; y++)
        for (x = 0; x < WELL_WIDTH; x++)
            if (well[y][x])
                puts(WELL_X + x * 2, y, BLACK, well[y][x], "  ");
            else
                puts(WELL_X + x * 2, y, BRIGHT, BLACK, "::");

    /* Ghost */
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[current.i][current.r][y][x])
                puts(WELL_X + current.x * 2 + x * 2, current.g + y,
                     TETRIS[current.i][current.r][y][x], BLACK, "::");

    /* Current */
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[current.i][current.r][y][x])
                puts(WELL_X + current.x * 2 + x * 2, current.y + y, BLACK,
                     TETRIS[current.i][current.r][y][x], "  ");
}

noreturn main()
{
    clear(BLACK);
    spawn();
    ghost();
    draw();

    bool debug = false;
    u64 tpms;
    u8 last_key;
loop:
    tpms = (u32) tps() / 1000;

    if (debug) {
        puts(0,  0, BRIGHT | GREEN, BLACK, "RTC sec:");
        puts(10, 0, GREEN,          BLACK, itoa(rtcs(), 16, 2));
        puts(0,  1, BRIGHT | GREEN, BLACK, "ticks/ms:");
        puts(10, 1, GREEN,          BLACK, itoa(tpms, 10, 10));
        puts(0,  2, BRIGHT | GREEN, BLACK, "key:");
        puts(10, 2, GREEN,          BLACK, itoa(last_key, 16, 2));
        puts(0,  3, BRIGHT | GREEN, BLACK, "i,r:");
        puts(10, 3, GREEN,          BLACK, itoa(current.i, 10, 1));
        putc(11, 3, GREEN,          BLACK, ',');
        puts(12, 3, GREEN,          BLACK, itoa(current.r, 10, 1));
        puts(0,  4, BRIGHT | GREEN, BLACK, "x,y,g:");
        puts(10, 4, GREEN,          BLACK, itoa(current.x, 10, 3));
        putc(13, 4, GREEN,          BLACK, ',');
        puts(14, 4, GREEN,          BLACK, itoa(current.y, 10, 3));
        putc(17, 4, GREEN,          BLACK, ',');
        puts(18, 4, GREEN,          BLACK, itoa(current.g, 10, 3));
        u32 i;
        for (i = 0; i < TIMER__LENGTH; i++) {
            puts(0,  5 + i, BRIGHT | GREEN, BLACK, "timer:");
            puts(10, 5 + i, GREEN,          BLACK, itoa(timers[i], 10, 10));
        }
    }

    bool updated = false;

    u8 key;
    if ((key = scan())) {
        last_key = key;
        switch(key) {
        case KEY_D:
            debug = !debug;
            clear(BLACK);
            break;
        case KEY_R:
            reset();
        case KEY_LEFT:
            move(-1, 0);
            break;
        case KEY_RIGHT:
            move(1, 0);
            break;
        case KEY_DOWN:
            move(0, 1);
            break;
        case KEY_UP:
            rotate();
            break;
        case KEY_ENTER:
            drop();
            break;
        }
        updated = true;
    }

    if (interval(TIMER_UPDATE, tpms * 1000)) {
        update();
        updated = true;
    }

    if (updated) {
        ghost();
        draw();
    }

    goto loop;
}
