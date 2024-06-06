#include "ui.h"

#include "app.h"
#include "embedded_files.h"
#include "image.h"
#include "imgui_config.h"
#include "lucide.h"

#include <array>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <string>

namespace mc
{
    void setColorsUI()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_Text]                  = ImVec4( 0.9490196108818054f, 0.95686274766922f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4( 0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f );
        style.Colors[ImGuiCol_WindowBg]              = ImVec4( 0.2117647081613541f, 0.2235294133424759f, 0.2470588237047195f, 1.0f );
        style.Colors[ImGuiCol_ChildBg]               = ImVec4( 0.1843137294054031f, 0.1921568661928177f, 0.2117647081613541f, 1.0f );
        style.Colors[ImGuiCol_PopupBg]               = ImVec4( 0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f );
        style.Colors[ImGuiCol_Border]                = ImVec4( 0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f );
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );
        style.Colors[ImGuiCol_FrameBg]               = ImVec4( 0.3098039329051971f, 0.3294117748737335f, 0.3607843220233917f, 1.0f );
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4( 0.3098039329051971f, 0.3294117748737335f, 0.3607843220233917f, 1.0f );
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4( 0.3450980484485626f, 0.3960784375667572f, 0.9490196108818054f, 1.0f );
        style.Colors[ImGuiCol_TitleBg]               = ImVec4( 0.1843137294054031f, 0.1921568661928177f, 0.2117647081613541f, 1.0f );
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4( 0.125490203499794f, 0.1333333402872086f, 0.1450980454683304f, 1.0f );
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4( 0.125490203499794f, 0.1333333402872086f, 0.1450980454683304f, 1.0f );
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4( 0.125490203499794f, 0.1333333402872086f, 0.1450980454683304f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4( 0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f );
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4( 0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4( 0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f );
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4( 0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f );
        style.Colors[ImGuiCol_CheckMark]             = ImVec4( 0.2313725501298904f, 0.6470588445663452f, 0.364705890417099f, 1.0f );
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
        style.Colors[ImGuiCol_Button]                = ImVec4( 0.3098039329051971f, 0.3294117748737335f, 0.3607843220233917f, 1.0f );
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4( 0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4( 0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_Header]                = ImVec4( 0.3098039329051971f, 0.3294117748737335f, 0.3607843220233917f, 1.0f );
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4( 0.407843142747879f, 0.4274509847164154f, 0.4509803950786591f, 1.0f );
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4( 0.407843142747879f, 0.4274509847164154f, 0.4509803950786591f, 1.0f );
        style.Colors[ImGuiCol_Separator]             = ImVec4( 0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f );
        style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4( 0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f );
        style.Colors[ImGuiCol_SeparatorActive]       = ImVec4( 0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f );
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4( 0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.0f );
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4( 0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4( 0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_Tab]                   = ImVec4( 0.1843137294054031f, 0.1921568661928177f, 0.2117647081613541f, 1.0f );
        style.Colors[ImGuiCol_TabHovered]            = ImVec4( 0.2352941185235977f, 0.2470588237047195f, 0.2705882489681244f, 1.0f );
        style.Colors[ImGuiCol_TabActive]             = ImVec4( 0.2588235437870026f, 0.2745098173618317f, 0.3019607961177826f, 1.0f );
        style.Colors[ImGuiCol_TabUnfocused]          = ImVec4( 0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f );
        style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4( 0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f );
        style.Colors[ImGuiCol_PlotLines]             = ImVec4( 0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f );
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4( 1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f );
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4( 0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f );
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4( 1.0f, 0.6000000238418579f, 0.0f, 1.0f );
        style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4( 0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f );
        style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4( 0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f );
        style.Colors[ImGuiCol_TableBorderLight]      = ImVec4( 0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f );
        style.Colors[ImGuiCol_TableRowBg]            = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );
        style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4( 1.0f, 1.0f, 1.0f, 0.05999999865889549f );
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4( 0.05098039284348488f, 0.4196078479290009f, 0.8588235378265381f, 1.0f );
        style.Colors[ImGuiCol_DragDropTarget]        = ImVec4( 0.3450980484485626f, 0.3960784375667572f, 0.9490196108818054f, 1.0f );
        style.Colors[ImGuiCol_NavHighlight]          = ImVec4( 0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f );
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.0f, 1.0f, 1.0f, 0.699999988079071f );
        style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4( 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f );
        style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4( 0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f );
    }

    void setStylesUI( float dpiFactor )
    {
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

        style.ScaleAllSizes( dpiFactor );

        ImGui::GetIO().Fonts->Clear();

        // configure fonts
        ImFontConfig configRoboto;
        configRoboto.FontDataOwnedByAtlas = false;
        configRoboto.OversampleH          = 2;
        configRoboto.OversampleV          = 2;
        configRoboto.RasterizerMultiply   = 1.5f;
        configRoboto.GlyphExtraSpacing    = ImVec2( 1.0f, 0 );
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<uint8_t*>( Roboto_ttf ), Roboto_ttf_size, 16.0f * dpiFactor, &configRoboto );

        ImFontConfig configLucide;
        configLucide.FontDataOwnedByAtlas = false;
        configLucide.OversampleH          = 2;
        configLucide.OversampleV          = 2;
        configLucide.MergeMode            = true;
        configLucide.GlyphMinAdvanceX     = 24.0f * dpiFactor; // Use if you want to make the icon monospaced

        // The calculation to get these glyphs perfectly centered on the y axis doesnt make sense but it works
        configLucide.GlyphOffset = ImVec2( 0.0f, 11.0f * dpiFactor - 5.8f );

        // Specify which icons we use
        // Need to specify or texture atlas will be too large and fail to upload to gpu on
        // lower end systems
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

        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<uint8_t*>( Lucide_ttf ), Lucide_ttf_size, 24.0f * dpiFactor, &configLucide, iconRanges.Data );

        ImGui::GetIO().Fonts->Build();
    }

    void initUI( AppContext* app )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        setColorsUI();
        setStylesUI( app->dpiFactor );

        ImGui_ImplSDL3_InitForOther( app->window );

        ImGui_ImplWGPU_InitInfo imguiWgpuInfo{};
        imguiWgpuInfo.Device             = app->device.Get();
        imguiWgpuInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>( app->colorFormat );
        ImGui_ImplWGPU_Init( &imguiWgpuInfo );
    }

    void drawUI( AppContext* app, const wgpu::RenderPassEncoder& renderPass )
    {
        static bool addImage = false;

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

        ImGui::Text( "Width: %d, Height: %d, Dpi factor: %.1f", app->width, app->height, app->dpiFactor );
        ImGui::Text( "Mouse x:%.1f Mouse y:%.1f Zoom:%.1f\n", app->viewParams.mousePos.x, app->viewParams.mousePos.y, app->viewParams.scale );
        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
        if( ImGui::Button( "Hard Quit" ) )
        {
            app->appQuit = true;
        }

        ImGui::SetNextWindowPos( ImVec2( 460, 20 ), ImGuiCond_FirstUseEver );
        ImGui::ShowDemoWindow();

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

        ImGui::Begin( "Toolbox", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus );
        {
            ImGui::SetWindowPos( glm::vec2( 10 ) * app->dpiFactor );
            ImGui::SetWindowSize( glm::vec2( 80, 300 ) * app->dpiFactor );

            const glm::vec2 buttonSize = glm::vec2( 50 ) * app->dpiFactor;

            ImGui::PushID( "Upload Image Button" );
            ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button] );
            if( ImGui::Button( ICON_LC_IMAGE_UP, buttonSize ) )
            {
                if( !addImage )
                {
                    loadImageFromFile( app );
                    addImage = true;
                }
            }
            else
            {
                addImage = false;
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
                if( ImGui::Button( tools[i].c_str(), buttonSize ) && states[i] != State::Other )
                {
                    app->state = states[i];
                }
                ImGui::PopStyleColor( 1 );
                ImGui::PopID();
            }
        }
        ImGui::End();

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        if( app->state == State::Cursor && app->dragType == CursorDragType::Select && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
        {
            drawList->AddRect( app->mouseDragStart, app->mouseWindowPos, ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] ) );
            drawList->AddRectFilled( app->mouseDragStart, app->mouseWindowPos, ImGui::GetColorU32( IM_COL32( 0, 130, 216, 50 ) ) );
        }

        if( app->selectionReady && app->layers.numSelected() > 0 && app->dragType == CursorDragType::Select )
        {
            glm::vec2 cornerTL = glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerBR = glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerTR = glm::vec2( cornerBR.x, cornerTL.y );
            glm::vec2 cornerBL = glm::vec2( cornerTL.x, cornerBR.y );

            float screenSpaceCenterX = ( cornerTL.x + cornerBR.x ) * 0.5f;

            glm::vec2 rotHandlePos = glm::vec2( screenSpaceCenterX, cornerTR.y - mc::RotateHandleHeight );

            ImU32 color = ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );

            drawList->AddRect( cornerTL, cornerBR, color, 0.0, 0, ceilf( app->dpiFactor ) );
            drawList->AddLine( rotHandlePos, ImVec2( screenSpaceCenterX, cornerTR.y ), color, ceilf( app->dpiFactor ) );

            color = glm::distance( app->mouseWindowPos, rotHandlePos ) < HandleHalfSize
                        ? ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered] )
                        : ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
            drawList->AddCircleFilled( rotHandlePos, HandleHalfSize * app->dpiFactor, color );

            color = glm::distance( app->mouseWindowPos, cornerBR ) < HandleHalfSize * app->dpiFactor
                        ? ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered] )
                        : ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
            drawList->AddCircleFilled( cornerBR, HandleHalfSize * app->dpiFactor, color );

            color = glm::distance( app->mouseWindowPos, cornerBL ) < HandleHalfSize * app->dpiFactor
                        ? ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered] )
                        : ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
            drawList->AddCircleFilled( cornerBL, HandleHalfSize * app->dpiFactor, color );

            color = glm::distance( app->mouseWindowPos, cornerTR ) < HandleHalfSize * app->dpiFactor
                        ? ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered] )
                        : ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
            drawList->AddCircleFilled( cornerTR, HandleHalfSize * app->dpiFactor, color );

            color = glm::distance( app->mouseWindowPos, cornerTL ) < HandleHalfSize * app->dpiFactor
                        ? ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered] )
                        : ImGui::GetColorU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
            drawList->AddCircleFilled( cornerTL, HandleHalfSize * app->dpiFactor, color );
        }

        ImGui::EndFrame();

        ImGui::Render();

        ImGui_ImplWGPU_RenderDrawData( ImGui::GetDrawData(), renderPass.Get() );
    }

    void processEventUI( const SDL_Event* event )
    {
        ImGui_ImplSDL3_ProcessEvent( event );
    }

    bool captureMouseUI()
    {
        return ImGui::GetIO().WantCaptureMouse;
    }

    void shutdownUI()
    {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace mc