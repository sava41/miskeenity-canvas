#include "events.h"

#include <SDL3/SDL.h>
#include <memory>

namespace mc
{

    void submitEvent( const Events& event, const EventData& data )
    {
        SDL_Event sdlEvent;
        sdlEvent.type      = SDL_EVENT_USER;
        sdlEvent.user.code = static_cast<int>( event );

        Event* eventData = reinterpret_cast<Event*>( &sdlEvent );
        eventData->data  = data;

        SDL_PushEvent( &sdlEvent );
    }


} // namespace mc