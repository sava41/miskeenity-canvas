#include "Events.h"

#include <SDL3/SDL.h>
#include <memory>

namespace mc
{

    void submitEvent( const Events& event, const EventDataType& type, void* userData )
    {
        SDL_Event user_event;
        user_event.type       = SDL_EVENT_USER;
        user_event.user.code  = static_cast<int>( event );
        user_event.user.data1 = new EventDataType( type );
        user_event.user.data2 = userData;

        SDL_PushEvent( &user_event );
    }
} // namespace mc