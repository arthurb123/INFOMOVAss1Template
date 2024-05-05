#include "precomp.h"
#include "game.h"

#define LINES		750
#define LINEFILE	"lines750.dat"
#define ITERATIONS	16

int lx1[LINES], ly1[LINES], lx2[LINES], ly2[LINES];			// lines: start and end coordinates
uint lc[LINES];												// lines: colors
int x1_, y1_, x2_, y2_;										// room for storing line backup
uint c_;													// line color backup
int fitness;												// similarity to reference image
int lidx = 0;												// current line to be mutated
float peak = 0;												// peak line rendering performance
Surface* reference, *backup;								// surfaces
Timer timer;

#define BYTE unsigned char
#define DWORD unsigned int
#define COLORREF DWORD
#define __int64 long long
#define RGB(r,g,b) ( ((DWORD)(BYTE)r)|((DWORD)((BYTE)g)<<8)|((DWORD)((BYTE)b)<<16) )
#define GetRValue(RGBColor) (BYTE) (RGBColor)
#define GetGValue(RGBColor) (BYTE) (((uint)RGBColor) >> 8)
#define GetBValue(RGBColor) (BYTE) (((uint)RGBColor) >> 16)

// -----------------------------------------------------------
// Mutate
// Randomly modify or replace one line.
// -----------------------------------------------------------
void MutateLine( int i )
{
	// backup the line before modifying it
	x1_ = lx1[i], y1_ = ly1[i];
	x2_ = lx2[i], y2_ = ly2[i];
	c_ = lc[i];
	do
	{
		if (rand() & 1)
		{
			// color mutation (50% probability)
			lc[i] = RandomUInt() & 0xffffff;
		}
		else if (rand() & 1)
		{
			// small mutation (25% probability)
			lx1[i] += RandomUInt() % 6 - 3, ly1[i] += RandomUInt() % 6 - 3;
			lx2[i] += RandomUInt() % 6 - 3, ly2[i] += RandomUInt() % 6 - 3;
			// ensure the line stays on the screen
			lx1[i] = min( SCRWIDTH - 1, max( 0, lx1[i] ) );
			lx2[i] = min( SCRWIDTH - 1, max( 0, lx2[i] ) );
			ly1[i] = min( SCRHEIGHT - 1, max( 0, ly1[i] ) );
			ly2[i] = min( SCRHEIGHT - 1, max( 0, ly2[i] ) );
		}
		else
		{
			// new line (25% probability)
			lx1[i] = RandomUInt() % SCRWIDTH, lx2[i] = RandomUInt() % SCRWIDTH;
			ly1[i] = RandomUInt() % SCRHEIGHT, ly2[i] = RandomUInt() % SCRHEIGHT;
		}
	} while ((abs( lx1[i] - lx2[i] ) < 3) || (abs( ly1[i] - ly2[i] ) < 3));
}

void UndoMutation( int i )
{
	// restore line i to the backuped state
	lx1[i] = x1_, ly1[i] = y1_;
	lx2[i] = x2_, ly2[i] = y2_;
	lc[i] = c_;
}

// -----------------------------------------------------------
// DrawWuLine
// Anti-aliased line rendering.
// Straight from:
// https://www.codeproject.com/Articles/13360/Antialiasing-Wu-Algorithm
// -----------------------------------------------------------
void DrawWuLine( Surface *screen, int X0, int Y0, int X1, int Y1, uint clrLine )
{
    /* Make sure the line runs top to bottom */
    if (Y0 > Y1)
    {
        int Temp = Y0; Y0 = Y1; Y1 = Temp;
        Temp = X0; X0 = X1; X1 = Temp;
    }

    /* Draw the initial pixel, which is always exactly intersected by
    the line and so needs no weighting */
    screen->Plot( X0, Y0, clrLine );

    int XDir, DeltaX = X1 - X0;
    if( DeltaX >= 0 )
    {
        XDir = 1;
    }
    else
    {
        XDir   = -1;
        DeltaX = 0 - DeltaX; /* make DeltaX positive */
    }

    /* Special-case horizontal, vertical, and diagonal lines, which
    require no weighting because they go right through the center of
    every pixel */
    int DeltaY = Y1 - Y0;

    unsigned short ErrorAdj;
    unsigned short ErrorAccTemp, Weighting;

    /* Line is not horizontal, diagonal, or vertical */
    unsigned short ErrorAcc = 0;  /* initialize the line error accumulator to 0 */

    BYTE rl = GetRValue( clrLine );
    BYTE gl = GetGValue( clrLine );
    BYTE bl = GetBValue( clrLine );
    double grayl = rl * 0.299 + gl * 0.587 + bl * 0.114;

    /* Is this an X-major or Y-major line? */
    if (DeltaY > DeltaX)
    {
    /* Y-major line; calculate 16-bit fixed-point fractional part of a
    pixel that X advances each time Y advances 1 pixel, truncating the
        result so that we won't overrun the endpoint along the X axis */
        ErrorAdj = ((unsigned long) DeltaX << 16) / (unsigned long) DeltaY;
        /* Draw all pixels other than the first and last */
        while (--DeltaY) {
            ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
            ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
            if (ErrorAcc <= ErrorAccTemp) {
                /* The error accumulator turned over, so advance the X coord */
                X0 += XDir;
            }
            Y0++; /* Y-major, so always advance Y */
                  /* The IntensityBits most significant bits of ErrorAcc give us the
                  intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
            Weighting = ErrorAcc >> 8;

            COLORREF clrBackGround = screen->pixels[X0 + Y0 * SCRWIDTH];
            BYTE rb = GetRValue( clrBackGround );
            BYTE gb = GetGValue( clrBackGround );
            BYTE bb = GetBValue( clrBackGround );
            double grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

            BYTE rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( rb - rl ) + rl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( rl - rb ) + rb ) ) );
            BYTE gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( gb - gl ) + gl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( gl - gb ) + gb ) ) );
            BYTE br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( bb - bl ) + bl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( bl - bb ) + bb ) ) );
            screen->Plot( X0, Y0, RGB( rr, gr, br ) );

            clrBackGround = screen->pixels[X0 + XDir + Y0 * SCRWIDTH];
            rb = GetRValue( clrBackGround );
            gb = GetGValue( clrBackGround );
            bb = GetBValue( clrBackGround );
            grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

            rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( rb - rl ) + rl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( rl - rb ) + rb ) ) );
            gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( gb - gl ) + gl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( gl - gb ) + gb ) ) );
            br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( bb - bl ) + bl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( bl - bb ) + bb ) ) );
            screen->Plot( X0 + XDir, Y0, RGB( rr, gr, br ) );
        }
        /* Draw the final pixel, which is always exactly intersected by the line
        and so needs no weighting */
        screen->Plot( X1, Y1, clrLine );
        return;
    }
    /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
    pixel that Y advances each time X advances 1 pixel, truncating the
    result to avoid overrunning the endpoint along the X axis */
    ErrorAdj = ((unsigned long) DeltaY << 16) / (unsigned long) DeltaX;
    /* Draw all pixels other than the first and last */
    while (--DeltaX) {
        ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
        ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
        if (ErrorAcc <= ErrorAccTemp) {
            /* The error accumulator turned over, so advance the Y coord */
            Y0++;
        }
        X0 += XDir; /* X-major, so always advance X */
                    /* The IntensityBits most significant bits of ErrorAcc give us the
                    intensity weighting for this pixel, and the complement of the
        weighting for the paired pixel */
        Weighting = ErrorAcc >> 8;

        COLORREF clrBackGround = screen->pixels[X0 + Y0 * SCRWIDTH];
        BYTE rb = GetRValue( clrBackGround );
        BYTE gb = GetGValue( clrBackGround );
        BYTE bb = GetBValue( clrBackGround );
        double grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

        BYTE rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( rb - rl ) + rl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( rl - rb ) + rb ) ) );
        BYTE gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( gb - gl ) + gl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( gl - gb ) + gb ) ) );
        BYTE br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( bb - bl ) + bl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?Weighting:(Weighting ^ 255)) ) / 255.0 * ( bl - bb ) + bb ) ) );

        screen->Plot( X0, Y0, RGB( rr, gr, br ) );

        clrBackGround = screen->pixels[X0 + (Y0 + 1 )* SCRWIDTH];
        rb = GetRValue( clrBackGround );
        gb = GetGValue( clrBackGround );
        bb = GetBValue( clrBackGround );
        grayb = rb * 0.299 + gb * 0.587 + bb * 0.114;

        rr = ( rb > rl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( rb - rl ) + rl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( rl - rb ) + rb ) ) );
        gr = ( gb > gl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( gb - gl ) + gl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( gl - gb ) + gb ) ) );
        br = ( bb > bl ? ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( bb - bl ) + bl ) ) : ( ( BYTE )( ( ( double )( grayl<grayb?(Weighting ^ 255):Weighting) ) / 255.0 * ( bl - bb ) + bb ) ) );

        screen->Plot( X0, Y0 + 1, RGB( rr, gr, br ) );
    }

    /* Draw the final pixel, which is always exactly intersected by the line
    and so needs no weighting */
    screen->Plot( X1, Y1, clrLine );
}

// -----------------------------------------------------------
// Fitness evaluation
// Compare current generation against reference image.
// -----------------------------------------------------------
int Game::Evaluate()
{
	const uint count = SCRWIDTH * SCRHEIGHT;
	__int64 diff = 0;
	for( uint i = 0; i < count; i++ )
	{
		uint src = screen->pixels[i];
		uint ref = reference->pixels[i];
		int r0 = (src >> 16) & 255, g0 = (src >> 8) & 255, b0 = src & 255;
		int r1 = ref >> 16, g1 = (ref >> 8) & 255, b1 = ref & 255;
		int dr = r0 - r1, dg = g0 - g1, db = b0 - b1;
		// calculate squared color difference;
		// take into account eye sensitivity to red, green and blue
		diff += 3 * dr * dr + 6 * dg * dg + db * db;
	}
	return (int)(diff >> 5);
}

// -----------------------------------------------------------
// Application initialization
// Load a previously saved generation, if available.
// -----------------------------------------------------------
void Game::Init()
{
	for (int i = 0; i < LINES; i++) MutateLine( i );
	FILE* f = fopen( LINEFILE, "rb" );
	if (f)
	{
		fread( lx1, 4, LINES, f );
		fread( ly1, 4, LINES, f );
		fread( lx2, 4, LINES, f );
		fread( ly2, 4, LINES, f );
		fread( lc, 4, LINES, f );
		fclose( f );
	}
	reference = new Surface( "assets/bird.png" );
	backup = new Surface( SCRWIDTH, SCRHEIGHT );
	memset( screen->pixels, 255, SCRWIDTH * SCRHEIGHT * 4 );
	for (int j = 0; j < LINES; j++)
	{
		DrawWuLine( screen, lx1[j], ly1[j], lx2[j], ly2[j], lc[j] );
	}
	fitness = Evaluate();
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::Tick( float /* deltaTime */ )
{
	timer.reset();
	int lineCount = 0;
	int iterCount = 0;
	// draw up to lidx
	memset( screen->pixels, 255, SCRWIDTH * SCRHEIGHT * 4 );
	for (int j = 0; j < lidx; j++, lineCount++)
	{
		DrawWuLine( screen, lx1[j], ly1[j], lx2[j], ly2[j], lc[j] );
	}
	int base = lidx;
	screen->CopyTo( backup, 0, 0 );
	// iterate and draw from lidx to end
	for (int k = 0; k < ITERATIONS; k++)
	{
		backup->CopyTo( screen, 0, 0 );
		MutateLine( lidx );
		for (int j = base; j < LINES; j++, lineCount++)
		{
			DrawWuLine( screen, lx1[j], ly1[j], lx2[j], ly2[j], lc[j] );
		}
		int diff = Evaluate();
		if (diff < fitness) fitness = diff; else UndoMutation( lidx );
		lidx = (lidx + 1) % LINES;
		iterCount++;
	}
	// stats
	char t[128];
	float elapsed = timer.elapsed();
	float lps = (float)lineCount / elapsed;
	peak = max( lps, peak );
	sprintf( t, "fitness: %i", fitness );
	screen->Bar( 0, SCRHEIGHT - 33, 130, SCRHEIGHT - 1, 0 );
	screen->Print( t, 2, SCRHEIGHT - 24, 0xffffff );
	sprintf( t, "lps:     %5.2fK", lps );
	screen->Print( t, 2, SCRHEIGHT - 16, 0xffffff );
	sprintf( t, "ips:     %5.2f", (iterCount * 1000) / elapsed );
	screen->Print( t, 2, SCRHEIGHT - 8, 0xffffff );
	sprintf( t, "peak:    %5.2f", peak );
	screen->Print( t, 2, SCRHEIGHT - 32, 0xffffff );
}

// -----------------------------------------------------------
// Application termination
// Save the current generation, so we can continue later.
// -----------------------------------------------------------
void Game::Shutdown()
{
	FILE* f = fopen( LINEFILE, "wb" );
	fwrite( lx1, 4, LINES, f );
	fwrite( ly1, 4, LINES, f );
	fwrite( lx2, 4, LINES, f );
	fwrite( ly2, 4, LINES, f );
	fwrite( lc, 4, LINES, f );
	fclose( f );
}