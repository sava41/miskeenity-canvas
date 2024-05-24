#include "ui.h"

#include "embedded_files.h"
#include "lucide.h"

#include <array>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <string>

namespace mc
{
    void initUI( AppContext* app )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        // configure fonts
        ImFontConfig configRoboto;
        configRoboto.FontDataOwnedByAtlas = false;
        configRoboto.OversampleH          = 2;
        configRoboto.OversampleV          = 2;
        configRoboto.RasterizerMultiply   = 1.5f;
        configRoboto.GlyphExtraSpacing    = ImVec2( 1.0f, 0 );
        io.Fonts->AddFontFromMemoryTTF( const_cast<uint8_t*>( Roboto_ttf ), Roboto_ttf_size, 18.0f, &configRoboto );

        ImFontConfig configLucide;
        configLucide.FontDataOwnedByAtlas = false;
        configLucide.OversampleH          = 2;
        configLucide.OversampleV          = 2;
        configLucide.MergeMode            = true;
        configLucide.GlyphMinAdvanceX     = 24.0f; // Use if you want to make the icon monospaced
        configLucide.GlyphOffset          = ImVec2( 0.0f, 5.0f );

        // Specify which icons we use
        // Need to specify or texture atlas will be too large and fail to upload to gpu on
        // lower systems
        ImFontGlyphRangesBuilder builder;
        builder.AddText( ICON_LC_GITHUB );
        builder.AddText( ICON_LC_IMAGE_UP );
        builder.AddText( ICON_LC_IMAGE_DOWN );
        builder.AddText( ICON_LC_ROTATE_CW );
        builder.AddText( ICON_LC_ARROW_UP_NARROW_WIDE );
        builder.AddText( ICON_LC_ARROW_DOWN_NARROW_WIDE );
        builder.AddText( ICON_LC_FLIP_HORIZONTAL_2 );
        builder.AddText( ICON_LC_FLIP_VERTICAL_2 );
        builder.AddText( ICON_LC_BRUSH );
        builder.AddText( ICON_LC_SQUARE_DASHED_MOUSE_POINTER );
        builder.AddText( ICON_LC_TRASH_2 );
        builder.AddText( ICON_LC_HAND );
        builder.AddText( ICON_LC_MOUSE_POINTER );
        builder.AddText( ICON_LC_UNDO );
        builder.AddText( ICON_LC_REDO );
        builder.AddText( ICON_LC_TYPE );
        builder.AddText( ICON_LC_INFO );
        builder.AddText( ICON_LC_CROP );

        ImVector<ImWchar> iconRanges;
        builder.BuildRanges( &iconRanges );

        io.Fonts->AddFontFromMemoryTTF( const_cast<uint8_t*>( Lucide_ttf ), Lucide_ttf_size, 24.0f, &configLucide, iconRanges.Data );

        io.Fonts->Build();

        // add style
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha                     = 1.0f;
        style.DisabledAlpha             = 1.0f;
        style.WindowPadding             = ImVec2( 12.0f, 12.0f );
        style.WindowRounding            = 3.0f;
        style.WindowBorderSize          = 0.0f;
        style.WindowMinSize             = ImVec2( 20.0f, 20.0f );
        style.WindowTitleAlign          = ImVec2( 0.5f, 0.5f );
        style.WindowMenuButtonPosition  = ImGuiDir_None;
        style.ChildRounding             = 3.0f;
        style.ChildBorderSize           = 1.0f;
        style.PopupRounding             = 3.0f;
        style.PopupBorderSize           = 1.0f;
        style.FramePadding              = ImVec2( 6.0f, 6.0f );
        style.FrameRounding             = 4.0f;
        style.FrameBorderSize           = 0.0f;
        style.ItemSpacing               = ImVec2( 12.0f, 6.0f );
        style.ItemInnerSpacing          = ImVec2( 6.0f, 3.0f );
        style.CellPadding               = ImVec2( 12.0f, 6.0f );
        style.IndentSpacing             = 20.0f;
        style.ColumnsMinSpacing         = 6.0f;
        style.ScrollbarSize             = 12.0f;
        style.ScrollbarRounding         = 3.0f;
        style.GrabMinSize               = 12.0f;
        style.GrabRounding              = 3.0f;
        style.TabRounding               = 3.0f;
        style.TabBorderSize             = 0.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition       = ImGuiDir_Right;
        style.ButtonTextAlign           = ImVec2( 0.5f, 0.5f );
        style.SelectableTextAlign       = ImVec2( 0.0f, 0.0f );

        style.Colors[ImGuiCol_Text]                  = ImVec4( 0.9f, 0.9f, 0.9f, 1.0f );
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4( 0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f );
        style.Colors[ImGuiCol_WindowBg]              = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_ChildBg]               = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_PopupBg]               = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_Border]                = ImVec4( 0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f );
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_FrameBg]               = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4( 0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f );
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4( 0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_TitleBg]               = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4( 0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4( 0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_CheckMark]             = ImVec4( 0.9f, 0.9f, 0.9f, 1.0f );
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4( 0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4( 0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_Button]                = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4( 0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f );
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4( 0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_Header]                = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4( 0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f );
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4( 0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_Separator]             = ImVec4( 0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f );
        style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4( 0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f );
        style.Colors[ImGuiCol_SeparatorActive]       = ImVec4( 0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f );
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4( 0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f );
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4( 0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_Tab]                   = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TabHovered]            = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_TabActive]             = ImVec4( 0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f );
        style.Colors[ImGuiCol_TabUnfocused]          = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4( 0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f );
        style.Colors[ImGuiCol_PlotLines]             = ImVec4( 0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f );
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4( 0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f );
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4( 1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4( 0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f );
        style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4( 0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f );
        style.Colors[ImGuiCol_TableBorderLight]      = ImVec4( 0.0f, 0.0f, 0.0f, 1.0f );
        style.Colors[ImGuiCol_TableRowBg]            = ImVec4( 0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f );
        style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4( 0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f );
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4( 0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f );
        style.Colors[ImGuiCol_DragDropTarget]        = ImVec4( 0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_NavHighlight]          = ImVec4( 0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4( 0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f );
        style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4( 0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f );

        ImGui_ImplSDL3_InitForOther( app->window );

        ImGui_ImplWGPU_InitInfo imguiWgpuInfo{};
        imguiWgpuInfo.Device             = app->device.Get();
        imguiWgpuInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>( app->colorFormat );
        ImGui_ImplWGPU_Init( &imguiWgpuInfo );
    }

    void drawUI( AppContext* app, const wgpu::RenderPassEncoder& renderPass )
    {
        static bool show_test_window    = true;
        static bool show_another_window = false;
        static bool show_quit_dialog    = false;
        static float f                  = 0.0f;

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

        ImGui::Text( "width: %d, height: %d, dpi: %.1f scale:%.1f\n pos x:%.1f\n pos y:%.1f\n", app->width, app->height,
                     static_cast<float>( app->width ) / static_cast<float>( app->bbwidth ), app->viewParams.scale, app->viewParams.mousePos.x,
                     app->viewParams.mousePos.y );
        ImGui::Text( "NOTE: programmatic quit isn't supported on mobile" );
        if( ImGui::Button( "Hard Quit" ) )
        {
            app->appQuit = true;
        }
        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if( show_another_window )
        {
            ImGui::SetNextWindowSize( ImVec2( 200, 100 ), ImGuiCond_FirstUseEver );
            ImGui::Begin( "Another Window", &show_another_window );
            ImGui::Text( "Hello" );
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowDemoWindow()
        if( show_test_window )
        {
            ImGui::SetNextWindowPos( ImVec2( 460, 20 ), ImGuiCond_FirstUseEver );
            ImGui::ShowDemoWindow();
        }

        // 4. Prepare and conditionally open the "Really Quit?" popup
        if( ImGui::BeginPopupModal( "Really Quit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "Do you really want to quit?\n" );
            ImGui::Separator();
            if( ImGui::Button( "OK", ImVec2( 120, 0 ) ) )
            {
                app->appQuit = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if( ImGui::Button( "Cancel", ImVec2( 120, 0 ) ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if( show_quit_dialog )
        {
            ImGui::OpenPopup( "Really Quit?" );
            show_quit_dialog = false;
        }

        ImGui::Begin( "toolbox", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                          ImGuiWindowFlags_NoBackground );
        {
            ImGui::SetWindowPos( ImVec2( 10, 10 ) );
            ImGui::SetWindowSize( ImVec2( 80, 300 ) );

            ImGui::PushID( "Upload Image Button" );
            ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button] );
            if( ImGui::Button( ICON_LC_IMAGE_UP, ImVec2( 50, 50 ) ) )
            {
                // upload image code goes goes here
            }
            ImGui::PopStyleColor( 1 );
            ImGui::PopID();

            std::array<std::string, 4> tools = { ICON_LC_MOUSE_POINTER, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND };
            std::array<State, 4> states      = { State::Cursor, State::Paint, State::Text, State::Pan };

            for( size_t i = 0; i < tools.size(); i++ )
            {
                ImGui::PushID( i );
                ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                if( app->state == states[i] )
                {
                    color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
                }

                ImGui::PushStyleColor( ImGuiCol_Button, color );
                if( ImGui::Button( tools[i].c_str(), ImVec2( 50, 50 ) ) && states[i] != State::Other )
                {
                    app->state = states[i];
                }
                ImGui::PopStyleColor( 1 );
                ImGui::PopID();
            }
        }
        ImGui::End();

        if( app->state == State::Cursor && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
        {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            drawList->AddRect( ImVec2( app->mouseDragStart.x, app->mouseDragStart.y ), ImVec2( app->mouseWindowPos.x, app->mouseWindowPos.y ),
                               ImGui::GetColorU32( IM_COL32( 0, 130, 216, 255 ) ) );
            drawList->AddRectFilled( ImVec2( app->mouseDragStart.x, app->mouseDragStart.y ), ImVec2( app->mouseWindowPos.x, app->mouseWindowPos.y ),
                                     ImGui::GetColorU32( IM_COL32( 0, 130, 216, 50 ) ) );
        }

        ImGui::EndFrame();

        ImGui::Render();

        ImGui_ImplWGPU_RenderDrawData( ImGui::GetDrawData(), renderPass.Get() );
    }

    void shutdownUI()
    {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace mc