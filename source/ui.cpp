#include "ui.h"

#include "app.h"
#include "color_theme.h"
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

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                 = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY900 );
        colors[ImGuiCol_TextDisabled]         = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY500 );
        colors[ImGuiCol_WindowBg]             = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY100 );
        colors[ImGuiCol_ChildBg]              = ImGui::ColorConvertU32ToFloat4( Spectrum::Static::NONE );
        colors[ImGuiCol_PopupBg]              = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY50 );
        colors[ImGuiCol_Border]               = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY300 );
        colors[ImGuiCol_BorderShadow]         = ImGui::ColorConvertU32ToFloat4( Spectrum::Static::NONE );
        colors[ImGuiCol_FrameBg]              = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_FrameBgHovered]       = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_FrameBgActive]        = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY50 );
        colors[ImGuiCol_TitleBg]              = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY300 );
        colors[ImGuiCol_TitleBgActive]        = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY200 );
        colors[ImGuiCol_TitleBgCollapsed]     = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_MenuBarBg]            = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY100 );
        colors[ImGuiCol_ScrollbarBg]          = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY100 );
        colors[ImGuiCol_ScrollbarGrab]        = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_ScrollbarGrabActive]  = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_CheckMark]            = ImGui::ColorConvertU32ToFloat4( Spectrum::GREEN700 );
        colors[ImGuiCol_SliderGrab]           = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_SliderGrabActive]     = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_Button]               = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_ButtonHovered]        = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_ButtonActive]         = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY50 );
        colors[ImGuiCol_Header]               = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY300 );
        colors[ImGuiCol_HeaderHovered]        = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_HeaderActive]         = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_Separator]            = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_SeparatorHovered]     = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_SeparatorActive]      = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_ResizeGrip]           = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_ResizeGripHovered]    = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_ResizeGripActive]     = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY700 );
        colors[ImGuiCol_PlotLines]            = ImGui::ColorConvertU32ToFloat4( Spectrum::BLUE400 );
        colors[ImGuiCol_PlotLinesHovered]     = ImGui::ColorConvertU32ToFloat4( Spectrum::BLUE600 );
        colors[ImGuiCol_PlotHistogram]        = ImGui::ColorConvertU32ToFloat4( Spectrum::BLUE400 );
        colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorConvertU32ToFloat4( Spectrum::BLUE600 );
        colors[ImGuiCol_Tab]                  = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY50 );
        colors[ImGuiCol_TabActive]            = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY400 );
        colors[ImGuiCol_TabHovered]           = ImGui::ColorConvertU32ToFloat4( Spectrum::GRAY600 );
        colors[ImGuiCol_TextSelectedBg]       = ImGui::ColorConvertU32ToFloat4( ( Spectrum::BLUE400 & 0x00FFFFFF ) | 0x33000000 );
        colors[ImGuiCol_DragDropTarget]       = ImGui::ColorConvertU32ToFloat4( Spectrum::YELLOW400 );
        colors[ImGuiCol_NavHighlight]         = ImGui::ColorConvertU32ToFloat4( ( Spectrum::GRAY900 & 0x00FFFFFF ) | 0x0A000000 );
    }

    void setStylesUI( float dpiFactor )
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha                     = 1.0f;
        style.DisabledAlpha             = 1.0f;
        style.WindowPadding             = ImVec2( 12.0f, 12.0f );
        style.WindowRounding            = 3.0f;
        style.WindowBorderSize          = 1.0f;
        style.WindowMinSize             = ImVec2( 20.0f, 20.0f );
        style.WindowTitleAlign          = ImVec2( 0.5f, 0.5f );
        style.WindowMenuButtonPosition  = ImGuiDir_None;
        style.ChildRounding             = 3.0f;
        style.ChildBorderSize           = 0.0f;
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
        style.TabBarBorderSize          = 2.0f;
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

            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );
            ImGui::PushID( "Upload Image Button" );
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

            ImGui::PopID();

            std::array<std::string, 4> tools = { ICON_LC_MOUSE_POINTER, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND };
            std::array<State, 4> states      = { State::Cursor, State::Paint, State::Text, State::Pan };

            for( size_t i = 0; i < tools.size(); i++ )
            {
                ImGui::PushID( i );
                ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                if( app->state == states[i] )
                {
                    color = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
                }

                ImGui::PushStyleColor( ImGuiCol_Button, color );
                if( ImGui::Button( tools[i].c_str(), buttonSize ) && states[i] != State::Other )
                {
                    app->state = states[i];
                }
                ImGui::PopStyleColor( 1 );
                ImGui::PopID();
            }
            ImGui::PopStyleColor( 1 );
            ImGui::PopStyleVar( 1 );
        }
        ImGui::End();

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        if( app->state == State::Cursor && app->dragType == CursorDragType::Select && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
        {
            drawList->AddRect( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE500 );
            drawList->AddRectFilled( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
        }

        if( app->selectionReady && app->layers.numSelected() > 0 && app->dragType == CursorDragType::Select )
        {
            glm::vec2 cornerTL = glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerBR = glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerTR = glm::vec2( cornerBR.x, cornerTL.y );
            glm::vec2 cornerBL = glm::vec2( cornerTL.x, cornerBR.y );

            float screenSpaceCenterX = ( cornerTL.x + cornerBR.x ) * 0.5f;

            glm::vec2 rotHandlePos = glm::vec2( screenSpaceCenterX, cornerTR.y - mc::RotateHandleHeight );


            drawList->AddRect( cornerTL, cornerBR, Spectrum::PURPLE400, 0.0, 0, ceilf( app->dpiFactor ) );
            drawList->AddLine( rotHandlePos, ImVec2( screenSpaceCenterX, cornerTR.y ), Spectrum::PURPLE400, ceilf( app->dpiFactor ) );

            ImU32 color = Spectrum::PURPLE400;

            drawList->AddCircleFilled( rotHandlePos, HandleHalfSize * app->dpiFactor, Spectrum::PURPLE400 );
            color = glm::distance( app->mouseWindowPos, rotHandlePos ) < HandleHalfSize ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( rotHandlePos, HandleHalfSize * app->dpiFactor - ceilf( app->dpiFactor ), color );

            drawList->AddCircleFilled( cornerBR, HandleHalfSize * app->dpiFactor, Spectrum::PURPLE400 );
            color = glm::distance( app->mouseWindowPos, cornerBR ) < HandleHalfSize * app->dpiFactor ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerBR, HandleHalfSize * app->dpiFactor - ceilf( app->dpiFactor ), color );

            drawList->AddCircleFilled( cornerBL, HandleHalfSize * app->dpiFactor, Spectrum::PURPLE400 );
            color = glm::distance( app->mouseWindowPos, cornerBL ) < HandleHalfSize * app->dpiFactor ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerBL, HandleHalfSize * app->dpiFactor - ceilf( app->dpiFactor ), color );

            drawList->AddCircleFilled( cornerTR, HandleHalfSize * app->dpiFactor, Spectrum::PURPLE400 );
            color = glm::distance( app->mouseWindowPos, cornerTR ) < HandleHalfSize * app->dpiFactor ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerTR, HandleHalfSize * app->dpiFactor - ceilf( app->dpiFactor ), color );

            drawList->AddCircleFilled( cornerTL, HandleHalfSize * app->dpiFactor, Spectrum::PURPLE400 );
            color = glm::distance( app->mouseWindowPos, cornerTL ) < HandleHalfSize * app->dpiFactor ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerTL, HandleHalfSize * app->dpiFactor - ceilf( app->dpiFactor ), color );
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