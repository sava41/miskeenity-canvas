#include "ui.h"

#include "app.h"
#include "battery/embed.hpp"
#include "color_theme.h"
#include "events.h"
#include "imgui_config.h"
#include "lucide.h"

#include <array>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>

namespace mc
{
    // Just a few global variables for the ui ;)
    Mode g_appMode;
    MouseLocationUI g_mouseLocationUI;
    float g_uiScale        = 1.0;
    bool g_showAbout       = false;
    bool g_showHelp        = false;

    glm::vec3 g_paintColor = glm::vec3( 1.0, 0.0, 0.0 );
    float g_paintRadius    = 100;

    std::string g_inputTextString     = "Miskeen";
    glm::vec3 g_inputTextColor        = glm::vec3( 0.0, 0.0, 0.0 );
    float g_inputTextOutline          = 0.0;
    glm::vec3 g_inputTextOutlineColor = glm::vec3( 1.0, 1.0, 1.0 );

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

    void setStylesUI( const AppContext* app )
    {
        // due to how SDL/Imgui work, we want to scale the UI only when pixel width/height == window width/height
        if( app->width != app->bbwidth && app->height != app->bbheight )
        {
            g_uiScale = 1.0;
        }
        else
        {
            g_uiScale = app->dpiFactor;
        }

        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha                     = 1.0f;
        style.DisabledAlpha             = 1.0f;
        style.WindowPadding             = glm::vec2( 12.0f, 12.0f );
        style.WindowRounding            = 3.0f;
        style.WindowBorderSize          = 0.0f;
        style.WindowMinSize             = glm::vec2( 20.0f, 20.0f );
        style.WindowTitleAlign          = glm::vec2( 0.5f, 0.5f );
        style.WindowMenuButtonPosition  = ImGuiDir_None;
        style.ChildRounding             = 3.0f;
        style.ChildBorderSize           = 0.0f;
        style.PopupRounding             = 3.0f;
        style.PopupBorderSize           = 1.0f;
        style.FramePadding              = glm::vec2( 6.0f, 6.0f );
        style.FrameRounding             = 4.0f;
        style.FrameBorderSize           = 0.0f;
        style.ItemSpacing               = glm::vec2( 12.0f, 6.0f );
        style.ItemInnerSpacing          = glm::vec2( 6.0f, 3.0f );
        style.CellPadding               = glm::vec2( 12.0f, 6.0f );
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
        style.ButtonTextAlign           = glm::vec2( 0.5f, 0.5f );
        style.SelectableTextAlign       = glm::vec2( 0.0f, 0.0f );
        style.HoverDelayNormal          = 0.8;

        style.ScaleAllSizes( g_uiScale );

        ImGui::GetIO().Fonts->Clear();

        // configure fonts
        ImFontConfig configRoboto;
        configRoboto.FontDataOwnedByAtlas = false;
        configRoboto.OversampleH          = 2;
        configRoboto.OversampleV          = 2;
        configRoboto.GlyphExtraSpacing    = glm::vec2( 1.0f, 0 );
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<char*>( b::embed<"./resources/fonts/Roboto.ttf">().data() ),
                                                    b::embed<"./resources/fonts/Roboto.ttf">().size(), 17.0f * g_uiScale, &configRoboto );

        ImFontConfig configLucide;
        configLucide.FontDataOwnedByAtlas = false;
        configLucide.OversampleH          = 2;
        configLucide.OversampleV          = 2;
        configLucide.MergeMode            = true;
        configLucide.GlyphMinAdvanceX     = 24.0f * g_uiScale; // Use if you want to make the icon monospaced

        // The calculation to get these glyphs perfectly centered on the y axis doesnt make sense but it works
        configLucide.GlyphOffset = glm::vec2( 0.0f, 11.0f * g_uiScale - 5.8f );

        // Specify which icons we use
        // Need to specify or texture atlas will be too large and fail to upload to gpu on
        // lower end systems
        ImFontGlyphRangesBuilder builder;
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
        builder.AddText( ICON_LC_ALIGN_CENTER );
        builder.AddText( ICON_LC_ALIGN_LEFT );
        builder.AddText( ICON_LC_ALIGN_RIGHT );
        builder.AddText( ICON_LC_SHAPES );

        ImVector<ImWchar> iconRanges;
        builder.BuildRanges( &iconRanges );

        // To reduce executable size we use a subset of lucide
        // To add extra glyphs you need to update used_lucide_unicodes.txt and generate a new Lucide_compact.ttf with the following command
        // `pyftsubset Lucide.ttf --output-file=Lucide_compact.ttf --unicodes-file=used_lucide_unicodes.txt`
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<char*>( b::embed<"./resources/fonts/Lucide_compact.ttf">().data() ),
                                                    b::embed<"./resources/fonts/Lucide_compact.ttf">().size(), 24.0f * g_uiScale, &configLucide,
                                                    iconRanges.Data );
        ImGui::GetIO().Fonts->Build();
    }

    void initUI( const AppContext* app )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().LogFilename = nullptr;

        setColorsUI();
        setStylesUI( app );

        ImGui_ImplSDL3_InitForOther( app->window );

        ImGui_ImplWGPU_InitInfo imguiWgpuInfo{};
        imguiWgpuInfo.Device             = app->device.Get();
        imguiWgpuInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>( app->colorFormat );
        ImGui_ImplWGPU_Init( &imguiWgpuInfo );
    }

    void computeMouseLocationUI( const AppContext* app, glm::vec2 mouseWindowPos )
    {
        g_mouseLocationUI = ImGui::GetIO().WantCaptureMouse ? MouseLocationUI::Window : MouseLocationUI::None;

        if( g_mouseLocationUI == MouseLocationUI::None && app->selectionReady && app->layers.numSelected() > 0 && app->dragType == CursorDragType::Select &&
            !app->mouseDown )
        {
            glm::vec2 cornerTL = glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerBR = glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerTR = glm::vec2( cornerBR.x, cornerTL.y );
            glm::vec2 cornerBL = glm::vec2( cornerTL.x, cornerBR.y );

            float screenSpaceCenterX = ( cornerTL.x + cornerBR.x ) * 0.5f;

            glm::vec2 rotHandlePos = glm::vec2( screenSpaceCenterX, cornerTR.y - mc::RotateHandleHeight );

            if( glm::distance( mouseWindowPos, rotHandlePos ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::RotateHandle;
            }
            else if( glm::distance( mouseWindowPos, cornerBR ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleBR;
            }
            else if( glm::distance( mouseWindowPos, cornerBL ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleBL;
            }
            else if( glm::distance( mouseWindowPos, cornerTR ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleTR;
            }
            else if( glm::distance( mouseWindowPos, cornerTL ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleTL;
            }
        }
    }

    void drawUI( const AppContext* app, const wgpu::RenderPassEncoder& renderPass )
    {
        computeMouseLocationUI( app, app->mouseWindowPos );

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

#ifndef NDEBUG
        ImGui::Text( "Width: %d, Height: %d, Dpi factor: %.1f", app->width, app->height, g_uiScale );
        ImGui::Text( "Mouse x:%.1f Mouse y:%.1f Zoom:%.1f\n", app->viewParams.mousePos.x, app->viewParams.mousePos.y, app->viewParams.scale );
        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
        if( ImGui::Button( "Hard Quit" ) )
        {
            submitEvent( Events::AppQuit );
        }

        ImGui::SetNextWindowPos( glm::vec2( 460, 20 ), ImGuiCond_FirstUseEver );
        ImGui::ShowDemoWindow();
#endif

        const glm::vec2 buttonSize = glm::vec2( 50 ) * g_uiScale;
        const float buttonSpacing  = 10.0 * g_uiScale;

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
        ImGui::Begin( "Menu Bar", nullptr,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
        ImGui::PopStyleVar( 1 );
        {
            ImGui::SetWindowPos( glm::vec2( buttonSpacing * 2 ) );
            ImGui::SetWindowSize( glm::vec2( buttonSize.x * 2 + buttonSpacing * 1, buttonSize.y ) );

            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );
            if( ImGui::Button( ICON_LC_INFO, buttonSize ) )
            {
                ImGui::OpenPopup( "Menu" );
            }

            ImGui::SameLine( 0.0, buttonSpacing );

            if( ImGui::Button( ICON_LC_IMAGE_DOWN, buttonSize ) )
            {
            }
            if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                ImGui::SetItemTooltip( "TODO: Save Image" );
            ImGui::PopStyleColor( 1 );
            ImGui::PopStyleVar( 1 );

            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing * 2, buttonSpacing * 2 + buttonSize.y + 5.0f ) );
            if( ImGui::BeginPopup( "Menu" ) )
            {

                if( ImGui::MenuItem( "About" ) )
                {
                    g_showAbout = true;
                }
                if( ImGui::MenuItem( "Github" ) )
                {
                    submitEvent( Events::OpenGithub );
                }
                if( ImGui::MenuItem( "Help" ) )
                {
                    g_showHelp = true;
                }

                ImGui::EndPopup();
            }
        }
        ImGui::End();

        // ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
        // ImGui::Begin( "Undo Redo", nullptr,
        //               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
        // ImGui::PopStyleVar( 1 );
        // {
        //     ImGui::SetWindowPos( glm::vec2( ( app->width - ( buttonSize.x * 2 + buttonSpacing * 3 ) ), app->height - buttonSpacing * 2 - buttonSize.y ) );
        //     ImGui::SetWindowSize( glm::vec2( buttonSize.x * 2 + buttonSpacing * 1, buttonSize.y ) );

        //     ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
        //     ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );
        //     if( ImGui::Button( ICON_LC_UNDO, buttonSize ) )
        //     {
        //     }

        //     ImGui::SameLine( 0.0, buttonSpacing );

        //     if( ImGui::Button( ICON_LC_REDO, buttonSize ) )
        //     {
        //     }

        //     ImGui::PopStyleColor( 1 );
        //     ImGui::PopStyleVar( 1 );
        // }
        // ImGui::End();

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
        ImGui::Begin( "Toolbox", nullptr,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
        ImGui::PopStyleVar( 1 );
        {
            ImGui::SetWindowPos( glm::vec2( ( app->width - ( buttonSize.x * 5 + buttonSpacing * 4 ) ) * 0.5, app->height - buttonSpacing * 2 - buttonSize.y ) );
            ImGui::SetWindowSize( glm::vec2( buttonSize.x * 5 + buttonSpacing * 4, buttonSize.y ) );

            ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );
            ImGui::PushID( "Upload Image Button" );
            if( ImGui::Button( ICON_LC_IMAGE_UP, buttonSize ) )
            {
                submitEvent( Events::LoadImage );
            }
            if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                ImGui::SetItemTooltip( "Upload Image" );

            ImGui::PopID();

            std::array<std::string, 4> tools    = { ICON_LC_MOUSE_POINTER, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND };
            std::array<std::string, 4> tooltips = { "Select [S]", "Paint Brush [B]", "TODO: Add Text [T]", "Pan [P]" };
            std::array<Mode, 4> modes           = { Mode::Cursor, Mode::Paint, Mode::Text, Mode::Pan };

            for( size_t i = 0; i < tools.size(); i++ )
            {
                ImGui::SameLine( 0.0, buttonSpacing );
                ImGui::PushID( i );
                ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                if( g_appMode == modes[i] )
                {
                    color = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
                }

                ImGui::PushStyleColor( ImGuiCol_Button, color );
                if( ImGui::Button( tools[i].c_str(), buttonSize ) && modes[i] != Mode::Other )
                {

                    // reset some things when the mode changes
                    g_inputTextString       = "Miskeen";
                    g_inputTextColor        = glm::vec3( 0.0, 0.0, 0.0 );
                    g_inputTextOutline      = 0.0;
                    g_inputTextOutlineColor = glm::vec3( 1.0, 1.0, 1.0 );

                    if( g_appMode == Mode::Paint || g_appMode == Mode::Text )
                    {
                        submitEvent( Events::ContextCancel );
                    }

                    g_appMode = modes[i];
                    submitEvent( Events::ModeChanged );
                }
                ImGui::PopStyleColor( 1 );
                if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                    ImGui::SetItemTooltip( tooltips[i].c_str() );
                ImGui::PopID();
            }
            ImGui::PopStyleColor( 1 );
            ImGui::PopStyleVar( 1 );
        }
        ImGui::End();

        if( app->layers.numSelected() > 0 )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
            ImGui::Begin( "Context Bar", nullptr,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
            ImGui::PopStyleVar( 1 );
            {
                ImGui::SetWindowPos( glm::vec2( ( app->width - ( buttonSize.x * 6 + buttonSpacing * 5 ) ) * 0.5, buttonSpacing * 2 ) );
                ImGui::SetWindowSize( glm::vec2( buttonSize.x * 6 + buttonSpacing * 5, buttonSize.y ) );

                ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );

                std::array<std::string, 6> tools    = { ICON_LC_ARROW_UP_NARROW_WIDE,
                                                        ICON_LC_ARROW_DOWN_NARROW_WIDE,
                                                        ICON_LC_FLIP_HORIZONTAL_2,
                                                        ICON_LC_FLIP_VERTICAL_2,
                                                        ICON_LC_CROP,
                                                        ICON_LC_TRASH_2 };
                std::array<std::string, 6> tooltips = { "Bring To Front", "Move To Back", "Flip Horizontal", "Flip Vertical", "TODO: Crop", "TODO: Delete" };
                std::array<mc::Events, 6> events    = { mc::Events::MoveFront,    mc::Events::MoveBack, mc::Events::FlipHorizontal,
                                                        mc::Events::FlipVertical, mc::Events::Crop,     mc::Events::Delete };

                for( size_t i = 0; i < tools.size(); i++ )
                {
                    if( ImGui::Button( tools[i].c_str(), buttonSize ) )
                    {
                        submitEvent( events[i] );
                    }
                    if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                        ImGui::SetItemTooltip( tooltips[i].c_str() );
                    ImGui::SameLine( 0.0, buttonSpacing );
                }
                ImGui::PopStyleColor( 1 );
                ImGui::PopStyleVar( 1 );
            }
            ImGui::End();
        }

        ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, glm::vec2( 0.5f, 0.5f ) );
        if( ImGui::BeginPopupModal( "About", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "Miskeenity Canvas %s", MC_GIT_HASH );
            ImGui::Text( "By Sava" );
            ImGui::Separator();

            if( ImGui::Button( "OK", ImVec2( 120, 0 ) ) )
            {
                ImGui::CloseCurrentPopup();
                g_showAbout = false;
            }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
        if( g_showAbout )
        {
            ImGui::OpenPopup( "About" );
        }

        ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, glm::vec2( 0.5f, 0.5f ) );
        if( ImGui::BeginPopupModal( "Help", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "Will add tool descriptions and a list of shortcut keys here at some point" );
            ImGui::Separator();

            if( ImGui::Button( "OK", ImVec2( 120, 0 ) ) )
            {
                ImGui::CloseCurrentPopup();
                g_showHelp = false;
            }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
        if( g_showHelp )
        {
            ImGui::OpenPopup( "Help" );
        }

        if( g_appMode == mc::Mode::Paint )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 180 * g_uiScale - buttonSpacing ), ImGuiCond_FirstUseEver );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 180.0 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Paint Brush Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {
                ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );

                ImGui::SliderFloat( "##Paint Radius", &g_paintRadius, 0.0f, 200.0f );

                ImGui::ColorEdit3( "##Paint Color", &g_paintColor.r );

                ImGui::PopItemWidth();

                ImGui::SeparatorText( "" );

                float width = ( ImGui::GetContentRegionAvail().x - 8 ) * 0.5;
                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    g_appMode = Mode::Cursor;
                    submitEvent( Events::ContextAccept );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    g_appMode = Mode::Cursor;
                    submitEvent( Events::ContextCancel );
                }
            }
            ImGui::End();
        }
        else if( g_appMode == mc::Mode::Text )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 435 * g_uiScale - buttonSpacing ), ImGuiCond_FirstUseEver );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 435.0 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Text Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {
                const char* items[]          = { "Arial", "Times New Roman", "Impact" };
                static int item_selected_idx = 0; // Here we store our selection data as an index.

                // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
                const char* combo_preview_value = items[item_selected_idx];

                ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );

                float width = 35 * g_uiScale;
                ImGui::Button( ICON_LC_ALIGN_LEFT, glm::vec2( width, 0.0 ) );
                ImGui::SameLine( 0.0, 8.0 );
                ImGui::Button( ICON_LC_ALIGN_RIGHT, glm::vec2( width, 0.0 ) );
                ImGui::SameLine( 0.0, 8.0 );
                ImGui::Button( ICON_LC_ALIGN_CENTER, glm::vec2( width, 0.0 ) );

                ImGui::InputTextMultiline( "##Input String", const_cast<char*>( g_inputTextString.c_str() ), g_inputTextString.capacity() + 1,
                                           glm::vec2( ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 5 ), ImGuiInputTextFlags_CallbackResize,
                                           []( ImGuiInputTextCallbackData* data ) -> int
                                           {
                                               if( data->EventFlag == ImGuiInputTextFlags_CallbackResize )
                                               {
                                                   g_inputTextString.resize( data->BufTextLen );
                                                   data->Buf = const_cast<char*>( g_inputTextString.c_str() );
                                               }
                                               return 0;
                                           } );


                ImGui::SeparatorText( "Font" );

                if( ImGui::BeginCombo( "", combo_preview_value ) )
                {
                    for( int n = 0; n < IM_ARRAYSIZE( items ); n++ )
                    {
                        const bool is_selected = ( item_selected_idx == n );
                        if( ImGui::Selectable( items[n], is_selected ) )
                            item_selected_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if( is_selected )
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::ColorEdit3( "##Text Color", &g_inputTextColor.r );

                ImGui::SeparatorText( "Outline" );

                ImGui::SliderFloat( "##Text Outline", &g_inputTextOutline, 0.0f, 2.0f );
                ImGui::ColorEdit3( "##Text Outline Color", &g_inputTextOutlineColor.r );

                ImGui::PopItemWidth();

                ImGui::SeparatorText( "" );

                width = ( ImGui::GetContentRegionAvail().x - 8 ) / 2.0;
                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    g_appMode = Mode::Cursor;
                    submitEvent( Events::ContextAccept );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    g_appMode = Mode::Cursor;
                    submitEvent( Events::ContextCancel );
                }
            }
            ImGui::End();
        }

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        if( g_appMode == Mode::Cursor && app->dragType == CursorDragType::Select && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
        {
            drawList->AddRect( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE500 );
            drawList->AddRectFilled( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
        }

        if( app->layers.numSelected() > 0 && app->dragType == CursorDragType::Select )
        {
            glm::vec2 cornerTL = glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerBR = glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerTR = glm::vec2( cornerBR.x, cornerTL.y );
            glm::vec2 cornerBL = glm::vec2( cornerTL.x, cornerBR.y );

            float screenSpaceCenterX = ( cornerTL.x + cornerBR.x ) * 0.5f;

            glm::vec2 rotHandlePos = glm::vec2( screenSpaceCenterX, cornerTR.y - mc::RotateHandleHeight );

            drawList->AddRect( cornerTL, cornerBR, Spectrum::PURPLE400, 0.0, 0, ceilf( g_uiScale ) );
            drawList->AddLine( rotHandlePos, glm::vec2( screenSpaceCenterX, cornerTR.y ), Spectrum::PURPLE400, ceilf( g_uiScale ) );

            ImU32 color = Spectrum::PURPLE400;

            drawList->AddCircleFilled( rotHandlePos, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::RotateHandle ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( rotHandlePos, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( cornerBR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerBR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( cornerBL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerBL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( cornerTR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerTR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( cornerTL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( cornerTL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );
        }

        if( g_appMode == Mode::Paint && g_mouseLocationUI == mc::MouseLocationUI::None )
        {
            drawList->AddCircle( app->mouseWindowPos, g_paintRadius * app->viewParams.scale, Spectrum::PURPLE400, 600, ceilf( g_uiScale ) );
        }

        ImGui::Render();

        ImGui_ImplWGPU_RenderDrawData( ImGui::GetDrawData(), renderPass.Get() );
    }

    void processEventUI( const AppContext* app, const SDL_Event* event )
    {
        ImGui_ImplSDL3_ProcessEvent( event );
    }

    void shutdownUI()
    {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    MouseLocationUI getMouseLocationUI()
    {
        return g_mouseLocationUI;
    }

    glm::vec3 getPaintColor()
    {
        return g_paintColor;
    }

    float getPaintRadius()
    {
        return g_paintRadius;
    }

    const std::string* getInputTextString()
    {
        return &g_inputTextString;
    }

    glm::vec3 getInputTextColor()
    {
        return g_inputTextColor;
    }

    glm::vec3 getInputTextOutlineColor()
    {
        return g_inputTextOutlineColor;
    }

    float getInputTextOutline()
    {
        return g_inputTextOutline;
    }

    Mode getAppMode()
    {
        return g_appMode;
    }
} // namespace mc