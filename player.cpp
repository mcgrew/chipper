/* How to play game music files with Music_Player (requires SDL library)

Run program with path to a game music file.

Left/Right  Change track
Space       Pause/unpause
E           Normal/slight stereo echo/more stereo echo
A			Enable/disable accurate emulation
-/=         Adjust tempo
1-9         Toggle channel on/off
0           Reset tempo and turn channels back on */

int const scope_width = 512;

#include "Music_Player.h"
#include "Audio_Scope.h"
#include "Playlist.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "SDL.h"

void handle_error( const char* );

static bool paused;
static Audio_Scope* scope;
static Music_Player* player;
static short scope_buf [scope_width * 2];

static void init()
{
	// Start SDL
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
		exit( EXIT_FAILURE );
	atexit( SDL_Quit );
	SDL_EnableKeyRepeat( 500, 80 );
	
	// Init scope
	scope = new Audio_Scope;
	if ( !scope )
		handle_error( "Out of memory" );
	if ( scope->init( scope_width, 256 ) )
		handle_error( "Couldn't initialize scope" );
	memset( scope_buf, 0, sizeof scope_buf );
	
	// Create player
	player = new Music_Player;
	if ( !player )
		handle_error( "Out of memory" );
	handle_error( player->init() );
	player->set_scope_buffer( scope_buf, scope_width * 2 );
}

static void start_track( int track, const char* path )
{
	paused = false;
	handle_error( player->start_track( track - 1 ) );
	
	// update window title with track info
	
	long seconds = player->track_info().length / 1000;
	const char* game = player->track_info().game;
	if ( !*game )
	{
		// extract filename
		game = strrchr( path, '\\' ); // DOS
		if ( !game )
			game = strrchr( path, '/' ); // UNIX
		if ( !game )
			game = path;
		else
			game++; // skip path separator
	}
	
	char title [512];
	sprintf( title, "%s: %d/%d %s (%ld:%02ld)",
			game, track, player->track_count(), player->track_info().song,
			seconds / 60, seconds % 60 );
	SDL_WM_SetCaption( title, title );
}

int main( int argc, char** argv )
{
	init();
  std::vector<int> order;
  Playlist playlist;
  
  for (int i=1; i < argc; i++) order.push_back(i);
  int fileidx = 0;
  bool running = true;
  bool next_file = false;
	
  while ( running ) {
    next_file = false;
    // Load file
    if (fileidx >= order.size())
      exit(0);
    const char* path = argv [order[fileidx]];
    handle_error( player->load_file( path ) );
    start_track( 1, path );
    
    // Main loop
    int track = 1;
    double tempo = 1.0;
    double stereo_depth = 0.0;
    bool accurate = true;;
    int muting_mask = 0;
    while ( !next_file )
    {
      SDL_Delay( 1000 / 100 );
      
      // Update scope
      scope->draw( scope_buf, scope_width, 2 );
      
      // Automatically go to next track when current one ends
      if ( player->track_ended() )
      {
        if ( track < player->track_count() ) {
          start_track( ++track, path );
        } else if ( fileidx < order.size()-1 ) {
          fileidx++;
          break;
        } else {
          player->pause( paused = true );
        }
      }
      
      // Handle keyboard input
      SDL_Event e;
      while ( SDL_PollEvent( &e ) )
      {
        switch ( e.type )
        {
        case SDL_QUIT:
          running = false;
          next_file = true;
          break;
        
        case SDL_KEYDOWN:
          int key = e.key.keysym.sym;
          switch ( key )
          {
          case SDLK_q:
          case SDLK_ESCAPE: // quit
            running = false;
            break;
          
          case SDLK_LEFT: // prev track
            if ( !paused && !--track ) {
              track = 1;
              if ( fileidx > 0 ) {
                fileidx--;
                  next_file = true;
                break;
              }
            }
            start_track( track, path );
            break;
          
          case SDLK_RIGHT: // next track
            if ( track < player->track_count() )
              start_track( ++track, path );
            else if ( fileidx < order.size()-1 ) {
              fileidx++;
              next_file = true;
            }
            break;
          
          case SDLK_MINUS: // reduce tempo
            tempo -= 0.1;
            if ( tempo < 0.1 )
              tempo = 0.1;
            player->set_tempo( tempo );
            break;
          
          case SDLK_EQUALS: // increase tempo
            tempo += 0.1;
            if ( tempo > 2.0 )
              tempo = 2.0;
            player->set_tempo( tempo );
            break;
          
          case SDLK_SPACE: // toggle pause
            paused = !paused;
            player->pause( paused );
            break;
          
          case SDLK_a: // toggle accurate emulation
            accurate = !accurate;
            player->enable_accuracy( accurate );
            break;
          
          case SDLK_e: // toggle echo
            stereo_depth += 0.2;
            if ( stereo_depth > 0.5 )
              stereo_depth = 0;
            player->set_stereo_depth( stereo_depth );
            break;
          
          case SDLK_0: // reset tempo and muting
            tempo = 1.0;
            muting_mask = 0;
            player->set_tempo( tempo );
            player->mute_voices( muting_mask );
            break;
          
          default:
            if ( SDLK_1 <= key && key <= SDLK_9 ) // toggle muting
            {
              muting_mask ^= 1 << (key - SDLK_1);
              player->mute_voices( muting_mask );
            }
          }
        }
      }
    }
  }
	
	// Cleanup
	delete player;
	delete scope;
	
	return 0;
}

void handle_error( const char* error )
{
	if ( error )
	{
		// put error in window title
		char str [256];
		sprintf( str, "Error: %s", error );
		fprintf( stderr, "%s\n", str );
		SDL_WM_SetCaption( str, str );
		
		// wait for keyboard or mouse activity
		SDL_Event e;
		do
		{
			while ( !SDL_PollEvent( &e ) ) { }
		}
		while ( e.type != SDL_QUIT && e.type != SDL_KEYDOWN && e.type != SDL_MOUSEBUTTONDOWN );

		exit( EXIT_FAILURE );
	}
}
