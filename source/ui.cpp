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
    MouseLocationUI g_mouseLocationUI;
    float g_uiScale  = 1.0;
    bool g_showAbout = false;
    bool g_showHelp  = false;

    glm::vec3 g_paintColor = glm::vec3( 0.0, 0.0, 0.0 );
    float g_paintRadius    = 100;

    std::string g_inputTextString               = "";
    glm::vec3 g_inputTextColor                  = glm::vec3( 0.0, 0.0, 0.0 );
    float g_inputTextOutline                    = 0.0;
    glm::vec3 g_inputTextOutlineColor           = glm::vec3( 1.0, 1.0, 1.0 );
    FontManager::Alignment g_inputTextAlignment = FontManager::Alignment::Left;
    FontManager::Font g_inputTextFont           = FontManager::Arial;
    bool g_setinputTextFocus                    = false;

    bool g_saveWithTransparency = true;

    TransformBox g_transformBox;

    ImVector<ImWchar> g_cursorIconRanges;

    void changeModeUI( Mode newMode )
    {
        // reset some things when the mode changes
        g_inputTextString       = "";
        g_inputTextColor        = glm::vec3( 0.0, 0.0, 0.0 );
        g_inputTextOutline      = 0.0;
        g_inputTextOutlineColor = glm::vec3( 1.0, 1.0, 1.0 );
        g_paintColor            = glm::vec3( 0.0, 0.0, 0.0 );

        if( newMode == Mode::Text )
        {
            ImGui::SetWindowFocus( "Text Settings" );
            g_setinputTextFocus = true;
        }
    }

    void acceptEditModeChanges( Mode currentMode )
    {
        if( currentMode == Mode::Crop )
        {
            submitEvent( Events::ApplyCrop );
        }

        if( currentMode == Mode::Paint || currentMode == Mode::Text )
        {
            submitEvent( Events::MergeEditLayers );
        }

        if( currentMode == Mode::Cut || currentMode == Mode::SegmentCut )
        {
            submitEvent( Events::ApplyCut );
        }
    }

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
        colors[ImGuiCol_PlotLines]            = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
        colors[ImGuiCol_PlotLinesHovered]     = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE600 );
        colors[ImGuiCol_PlotHistogram]        = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
        colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE600 );
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
        style.DisabledAlpha             = 0.5f;
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
        configRoboto.GlyphExtraAdvanceX   = 1.0f;
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

        // To reduce executable size we use a subset of lucide
        // To add extra glyphs you need to update used_lucide_unicodes.txt and generate a new Lucide_compact.ttf with the following command
        // `pyftsubset Lucide.ttf --output-file=Lucide_compact.ttf --unicodes-file=used_lucide_unicodes.txt`
        ImFontGlyphRangesBuilder builder;
        builder.AddText( ICON_LC_IMAGE_UP );
        builder.AddText( ICON_LC_SAVE );
        builder.AddText( ICON_LC_LIST_END );
        builder.AddText( ICON_LC_LIST_START );
        builder.AddText( ICON_LC_FLIP_HORIZONTAL_2 );
        builder.AddText( ICON_LC_FLIP_VERTICAL_2 );
        builder.AddText( ICON_LC_BRUSH );
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
        builder.AddText( ICON_LC_SQUARE_BOTTOM_DASHED_SCISSORS );
        builder.AddText( ICON_LC_SQUARE_DASHED_MOUSE_POINTER );
        builder.AddText( ICON_LC_GROUP );

        ImVector<ImWchar> iconRanges;
        builder.BuildRanges( &iconRanges );

        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<char*>( b::embed<"./resources/fonts/Lucide_compact.ttf">().data() ),
                                                    b::embed<"./resources/fonts/Lucide_compact.ttf">().size(), 24.0f * g_uiScale, &configLucide,
                                                    iconRanges.Data );

        ImFontGlyphRangesBuilder cursorBuilder;
        cursorBuilder.AddText( ICON_LC_ROTATE_CW );
        cursorBuilder.AddText( ICON_LC_MOVE );
        cursorBuilder.AddText( ICON_LC_MOVE_DIAGONAL );
        cursorBuilder.BuildRanges( &g_cursorIconRanges );

        ImGui::GetIO().Fonts->AddFontFromMemoryTTF( const_cast<char*>( b::embed<"./resources/fonts/Lucide_compact.ttf">().data() ),
                                                    b::embed<"./resources/fonts/Lucide_compact.ttf">().size(), 24.0f * g_uiScale, &configLucide,
                                                    g_cursorIconRanges.Data );

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
        g_mouseLocationUI = MouseLocationUI::None;

        if( app->selectionReady && app->layers.numSelected() > 0 )
        {
            if( glm::distance( mouseWindowPos, g_transformBox.rotationHandle ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::RotateHandle;
            }
            else if( glm::distance( mouseWindowPos, g_transformBox.cornerHandleBR ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleBR;
            }
            else if( glm::distance( mouseWindowPos, g_transformBox.cornerHandleBL ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleBL;
            }
            else if( glm::distance( mouseWindowPos, g_transformBox.cornerHandleTR ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleTR;
            }
            else if( glm::distance( mouseWindowPos, g_transformBox.cornerHandleTL ) < HandleHalfSize * g_uiScale )
            {
                g_mouseLocationUI = MouseLocationUI::ScaleHandleTL;
            }
            else if( g_transformBox.insideBox )
            {
                g_mouseLocationUI = MouseLocationUI::MoveHandle;
            }
        }

        if( ImGui::GetIO().WantCaptureMouse )
        {
            g_mouseLocationUI = MouseLocationUI::Window;
        }
    }

    void computeTransformBox( const AppContext* app, glm::vec2 mouseWindowPos )
    {
        int imageSelection = app->layers.getSingleSelectedImage();
        if( imageSelection >= 0 )
        {
            Layer layer = app->layers.data()[imageSelection];

            // see if the layer is mirrored
            glm::vec3 cross = glm::cross( glm::vec3( layer.basisA, 0.0f ), glm::vec3( layer.basisB, 0.0f ) );
            float sign      = glm::sign( cross ).z;

            g_transformBox.cornerHandleTL =
                ( layer.offset - ( layer.basisA * sign + layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            g_transformBox.cornerHandleBR =
                ( layer.offset + ( layer.basisA * sign + layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            g_transformBox.cornerHandleTR =
                ( layer.offset + ( layer.basisA * sign - layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            g_transformBox.cornerHandleBL =
                ( layer.offset - ( layer.basisA * sign - layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;

            g_transformBox.rotationHandleBase = ( layer.offset - layer.basisB * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;

            g_transformBox.rotationHandle = g_transformBox.rotationHandleBase - glm::normalize( layer.basisB ) * RotateHandleHeight;

            auto isOnRightSide = []( const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& mouse ) -> bool
            {
                glm::vec2 a = glm::vec2( p2.x - p1.x, p2.y - p1.y );
                glm::vec2 b = glm::vec2( mouse.x - p2.x, mouse.y - p2.y );
                return ( a.x * b.y - b.x * a.y ) > 1.0f;
            };

            g_transformBox.insideBox = isOnRightSide( g_transformBox.cornerHandleTL, g_transformBox.cornerHandleTR, mouseWindowPos ) &&
                                       isOnRightSide( g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBR, mouseWindowPos ) &&
                                       isOnRightSide( g_transformBox.cornerHandleBR, g_transformBox.cornerHandleBL, mouseWindowPos ) &&
                                       isOnRightSide( g_transformBox.cornerHandleBL, g_transformBox.cornerHandleTL, mouseWindowPos );
        }
        else
        {

            g_transformBox.cornerHandleTL = glm::vec2( app->selectionAabb.z, app->selectionAabb.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            g_transformBox.cornerHandleBR = glm::vec2( app->selectionAabb.x, app->selectionAabb.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            g_transformBox.cornerHandleTR = glm::vec2( g_transformBox.cornerHandleBR.x, g_transformBox.cornerHandleTL.y );
            g_transformBox.cornerHandleBL = glm::vec2( g_transformBox.cornerHandleTL.x, g_transformBox.cornerHandleBR.y );

            g_transformBox.rotationHandleBase = ( g_transformBox.cornerHandleTL + g_transformBox.cornerHandleTR ) * 0.5f;

            g_transformBox.rotationHandle = glm::vec2( g_transformBox.rotationHandleBase.x, g_transformBox.rotationHandleBase.y - RotateHandleHeight );

            g_transformBox.insideBox = g_transformBox.cornerHandleTL.x < mouseWindowPos.x && g_transformBox.cornerHandleTL.y < mouseWindowPos.y &&
                                       g_transformBox.cornerHandleBR.x > mouseWindowPos.x && g_transformBox.cornerHandleBR.y > mouseWindowPos.y;
        }
    }

    void drawCursor( const glm::vec2& mousePos, const glm::vec2& cursorAngle, float scale, const Mode& appMode, bool mouseDown )
    {
        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        if( appMode == Mode::Paint )
        {
            switch( g_mouseLocationUI )
            {
            case MouseLocationUI::MoveHandle:
            case MouseLocationUI::RotateHandle:
            case MouseLocationUI::ScaleHandleTL:
            case MouseLocationUI::ScaleHandleTR:
            case MouseLocationUI::ScaleHandleBL:
            case MouseLocationUI::ScaleHandleBR:
            case MouseLocationUI::None:
                ImGui::SetMouseCursor( ImGuiMouseCursor_None );
                drawList->AddCircle( mousePos, g_paintRadius * scale, Spectrum::Static::BLACK, 600, ceilf( g_uiScale ) );
                break;
            default:
                ImGui::SetMouseCursor( ImGuiMouseCursor_Arrow );
                break;
            }
        }

        if( appMode == Mode::Cut )
        {
            switch( g_mouseLocationUI )
            {
            case MouseLocationUI::MoveHandle:
                ImGui::SetMouseCursor( ImGuiMouseCursor_None );
                drawList->AddCircle( mousePos, g_paintRadius * scale, Spectrum::Static::BLACK, 600, ceilf( g_uiScale ) );
                break;
            default:
                ImGui::SetMouseCursor( ImGuiMouseCursor_Arrow );
                break;
            }
        }

        if( appMode == Mode::Save )
        {
            ImGui::SetMouseCursor( ImGuiMouseCursor_Crosshair );
        }

        if( appMode == Mode::Crop )
        {
            if( !mouseDown )
            {
                switch( g_mouseLocationUI )
                {
                case MouseLocationUI::ScaleHandleTL:
                case MouseLocationUI::ScaleHandleBR:
                case MouseLocationUI::ScaleHandleTR:
                case MouseLocationUI::ScaleHandleBL:
                {
                    ImGui::SetMouseCursor( ImGuiMouseCursor_None );

                    glm::vec2 p1 = mousePos + glm::vec2( 1.0f, 1.0f ) * 12.0f * g_uiScale;
                    glm::vec2 p2 = mousePos + glm::vec2( -1.0f, 1.0f ) * 12.0f * g_uiScale;
                    glm::vec2 p3 = mousePos + glm::vec2( -1.0f, -1.0f ) * 12.0f * g_uiScale;
                    glm::vec2 p4 = mousePos + glm::vec2( 1.0f, -1.0f ) * 12.0f * g_uiScale;

                    // No idea how we associate g_cursorIconRanges to their respective icon so I'm just going to hardcode for now
                    const ImFontGlyph* glyph = ImGui::GetIO().Fonts->Fonts[0]->FindGlyph( g_cursorIconRanges[0] );
                    drawList->AddImageQuad( ImGui::GetIO().Fonts->TexID, p1, p2, p3, p4, glm::vec2( glyph->U1, glyph->V1 ), glm::vec2( glyph->U0, glyph->V1 ),
                                            glm::vec2( glyph->U0, glyph->V0 ), glm::vec2( glyph->U1, glyph->V0 ), Spectrum::Static::BLACK );
                }
                break;
                default:
                    ImGui::SetMouseCursor( ImGuiMouseCursor_Arrow );
                    break;
                }
            }
        }

        if( appMode == Mode::Cursor )
        {
            const ImFontGlyph* glyph = nullptr;
            glm::vec2 angle          = cursorAngle;

            if( !mouseDown )
            {

                switch( g_mouseLocationUI )
                {
                case MouseLocationUI::RotateHandle:
                {
                    // No idea how we associate g_cursorIconRanges to their respective icon so I'm just going to hardcode for now
                    glyph = ImGui::GetIO().Fonts->Fonts[0]->FindGlyph( g_cursorIconRanges[2] );
                    angle = glm::vec2( 1.0, 0.0 );
                }
                break;
                case MouseLocationUI::ScaleHandleTL:
                case MouseLocationUI::ScaleHandleBR:
                {
                    glyph = ImGui::GetIO().Fonts->Fonts[0]->FindGlyph( g_cursorIconRanges[4] );
                    angle = glm::vec2( -angle.y, angle.x );
                }
                break;
                case MouseLocationUI::ScaleHandleTR:
                case MouseLocationUI::ScaleHandleBL:
                {
                    glyph = ImGui::GetIO().Fonts->Fonts[0]->FindGlyph( g_cursorIconRanges[4] );
                }
                break;
                case MouseLocationUI::MoveHandle:
                {
                    glyph = ImGui::GetIO().Fonts->Fonts[0]->FindGlyph( g_cursorIconRanges[0] );
                    angle = glm::vec2( 1.0, 0.0 );
                }
                break;
                }
            }

            glm::vec2 perpendicular = glm::vec2( -angle.y, angle.x );
            glm::vec2 p1            = mousePos + ( perpendicular + angle ) * 12.0f * g_uiScale;
            glm::vec2 p2            = mousePos + ( perpendicular - angle ) * 12.0f * g_uiScale;
            glm::vec2 p3            = mousePos + ( -perpendicular - angle ) * 12.0f * g_uiScale;
            glm::vec2 p4            = mousePos + ( -perpendicular + angle ) * 12.0f * g_uiScale;


            if( glyph != nullptr )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_None );
                drawList->AddImageQuad( ImGui::GetIO().Fonts->TexID, p1, p2, p3, p4, glm::vec2( glyph->U1, glyph->V1 ), glm::vec2( glyph->U0, glyph->V1 ),
                                        glm::vec2( glyph->U0, glyph->V0 ), glm::vec2( glyph->U1, glyph->V0 ), Spectrum::Static::BLACK );
            }
            else
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_Arrow );
            }
        }
    }

    void drawShadedRectangleMask( int width, int height, glm::vec4 selectionAabb, const std::array<glm::vec2, 4>& points, ImDrawList* drawList )
    {
        std::array<glm::vec2, 4> pointsSorted = points;
        std::sort( pointsSorted.begin(), pointsSorted.end(),
                   []( const glm::vec2& a, const glm::vec2& b ) { return ( a.y < b.y ) || ( a.y == b.y && a.x < b.x ); } );

        bool flipMidPoints = pointsSorted[1].x > pointsSorted[2].x;

        // find outer bounds for our concave polygon and multiply by 2 to create some padding
        glm::vec2 screenCornerTL( std::min<float>( 0.0f, selectionAabb.z * 2.0 ), std::min<float>( 0.0f, selectionAabb.w * 2.0 ) );
        glm::vec2 screenCornerBR( std::max<float>( width, selectionAabb.x * 2.0 ), std::max<float>( height, selectionAabb.y * 2.0 ) );
        glm::vec2 screenCornerTR( std::max<float>( width, selectionAabb.x * 2.0 ), std::min<float>( 0.0f, selectionAabb.w * 2.0 ) );
        glm::vec2 screenCornerBL( std::min<float>( 0.0f, selectionAabb.z * 2.0 ), std::max<float>( height, selectionAabb.y * 2.0 ) );

        std::array<ImVec2, 10> pointsClockwise = { screenCornerTL,
                                                   screenCornerTR,
                                                   screenCornerBR,
                                                   screenCornerBL,
                                                   screenCornerTL,
                                                   pointsSorted[0],
                                                   pointsSorted[flipMidPoints ? 2 : 1],
                                                   pointsSorted[3],
                                                   pointsSorted[flipMidPoints ? 1 : 2],
                                                   pointsSorted[0] };

        drawList->AddConcavePolyFilled( pointsClockwise.data(), pointsClockwise.size(), Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
    }

    void drawUI( const AppContext* app, const wgpu::RenderPassEncoder& renderPass )
    {
        computeTransformBox( app, app->mouseWindowPos );
        computeMouseLocationUI( app, app->mouseWindowPos );

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

#ifndef NDEBUG
        ImGui::Text( "Width: %d, Height: %d, Dpi factor: %.1f", app->width, app->height, g_uiScale );
        ImGui::Text( "Mouse x:%.1f Mouse y:%.1f Zoom:%.1f\n", app->viewParams.mousePos.x, app->viewParams.mousePos.y, app->viewParams.scale );
        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
        ImGui::Text( "Num selected %d", app->layers.numSelected() );
        if( ImGui::Button( "Hard Quit" ) )
        {
            submitEvent( Events::AppQuit );
        }

        ImGui::SetNextWindowPos( glm::vec2( 460, 20 ), ImGuiCond_FirstUseEver );
        ImGui::ShowDemoWindow();
#endif

        const glm::vec2 buttonSize = glm::vec2( 50 ) * g_uiScale;
        const float buttonSpacing  = 10.0 * g_uiScale;

        if( app->mode != Mode::Save )
        {

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

                if( ImGui::Button( ICON_LC_SAVE, buttonSize ) )
                {
                    ImGui::OpenPopup( "Saving" );
                }

                if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                    ImGui::SetItemTooltip( "Save Image" );
                ImGui::PopStyleColor( 1 );
                ImGui::PopStyleVar( 1 );

                ImGui::SetNextWindowPos( glm::vec2( buttonSpacing * 3 + buttonSize.x, buttonSpacing * 2 + buttonSize.y + 5.0f ) );
                if( ImGui::BeginPopup( "Saving" ) )
                {

                    if( ImGui::MenuItem( "Save With Background" ) )
                    {
                        g_saveWithTransparency = false;

                        acceptEditModeChanges( app->mode );

                        submitEvent( Events::ChangeMode, { .mode = Mode::Save } );
                    }
                    if( ImGui::MenuItem( "Save With Transparency" ) )
                    {
                        g_saveWithTransparency = true;

                        acceptEditModeChanges( app->mode );

                        submitEvent( Events::ChangeMode, { .mode = Mode::Save } );
                    }

                    ImGui::EndPopup();
                }

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

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
            ImGui::Begin( "Undo Redo", nullptr,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
            ImGui::PopStyleVar( 1 );
            {
                ImGui::SetWindowPos( glm::vec2( ( app->width - ( buttonSize.x * 2 + buttonSpacing * 3 ) ), app->height - buttonSpacing * 2 - buttonSize.y ) );
                ImGui::SetWindowSize( glm::vec2( buttonSize.x * 2 + buttonSpacing * 1, buttonSize.y ) );

                ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );

                if( app->layerHistory.atBack() )
                {
                    ImGui::BeginDisabled();
                }
                if( ImGui::Button( ICON_LC_UNDO, buttonSize ) )
                {
                    submitEvent( Events::Undo );
                    submitEvent( Events::ComputeSelectionBbox );
                }
                if( app->layerHistory.atBack() )
                {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine( 0.0, buttonSpacing );

                if( app->layerHistory.atFront() )
                {
                    ImGui::BeginDisabled();
                }
                if( ImGui::Button( ICON_LC_REDO, buttonSize ) )
                {
                    submitEvent( Events::Redo );
                    submitEvent( Events::ComputeSelectionBbox );
                }
                if( app->layerHistory.atFront() )
                {
                    ImGui::EndDisabled();
                }

                ImGui::PopStyleColor( 1 );
                ImGui::PopStyleVar( 1 );
            }
            ImGui::End();

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
            ImGui::Begin( "Toolbox", nullptr,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
            ImGui::PopStyleVar( 1 );
            {
                ImGui::SetWindowPos(
                    glm::vec2( ( app->width - ( buttonSize.x * 5 + buttonSpacing * 4 ) ) * 0.5, app->height - buttonSpacing * 2 - buttonSize.y ) );
                ImGui::SetWindowSize( glm::vec2( buttonSize.x * 5 + buttonSpacing * 4, buttonSize.y ) );

                ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );
                ImGui::PushID( "Upload Image Button" );
                if( ImGui::Button( ICON_LC_IMAGE_UP, buttonSize ) )
                {
                    // if its currently in an edit mode, we treat it like pressing "accept"
                    acceptEditModeChanges( app->mode );

                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );

                    submitEvent( Events::LoadImage );
                }
                if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                    ImGui::SetItemTooltip( "Upload Image" );

                ImGui::PopID();

                std::array<std::string, 4> tools    = { ICON_LC_MOUSE_POINTER, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND };
                std::array<std::string, 4> tooltips = { "Select [S]", "Paint Brush [B]", "Add Text [T]", "Pan [P]" };
                std::array<Mode, 4> modes           = { Mode::Cursor, Mode::Paint, Mode::Text, Mode::Pan };

                for( size_t i = 0; i < tools.size(); i++ )
                {
                    ImGui::SameLine( 0.0, buttonSpacing );
                    ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                    if( app->mode == modes[i] )
                    {
                        color = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
                    }

                    ImGui::PushStyleColor( ImGuiCol_Button, color );
                    if( ImGui::Button( tools[i].c_str(), buttonSize ) )
                    {
                        // if its currently in an edit mode, we treat it like pressing "accept"
                        acceptEditModeChanges( app->mode );

                        submitEvent( Events::ChangeMode, { .mode = modes[i] } );
                    }
                    ImGui::PopStyleColor( 1 );
                    if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                        ImGui::SetItemTooltip( tooltips[i].c_str() );
                }
                ImGui::PopStyleColor( 1 );
                ImGui::PopStyleVar( 1 );
            }
            ImGui::End();
        }

        if( app->layers.numSelected() > 0 && app->mode == Mode::Cursor )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, glm::vec2( 0.0 ) );
            ImGui::Begin( "Context Bar", nullptr,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus );
            ImGui::PopStyleVar( 1 );
            {
                ImGui::SetWindowPos( glm::vec2( ( app->width - ( buttonSize.x * 5 + buttonSpacing * 4 ) ) * 0.5, buttonSpacing * 2 ) );
                ImGui::SetWindowSize( glm::vec2( buttonSize.x * 16 + buttonSpacing * 15, buttonSize.y ) );

                ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.0 );
                ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Spectrum::PURPLE700 );

                std::array<std::string, 5> tools    = { ICON_LC_TRASH_2, ICON_LC_LIST_START, ICON_LC_LIST_END, ICON_LC_FLIP_HORIZONTAL_2,
                                                        ICON_LC_FLIP_VERTICAL_2 };
                std::array<std::string, 5> tooltips = { "Delete", "Bring To Front", "Move To Back", "Flip Horizontal", "Flip Vertical" };
                std::array<Events, 5> toolEvents    = { Events::Delete, Events::MoveFront, Events::MoveBack, Events::FlipHorizontal, Events::FlipVertical };

                for( size_t i = 0; i < tools.size(); i++ )
                {
                    if( ImGui::Button( tools[i].c_str(), buttonSize ) )
                    {
                        submitEvent( toolEvents[i] );
                    }
                    if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                        ImGui::SetItemTooltip( tooltips[i].c_str() );
                    ImGui::SameLine( 0.0, buttonSpacing );
                }

                ImGui::SameLine( 0.0, buttonSpacing * 5 );

                if( app->layers.getSingleSelectedImage() >= 0 )
                {
                    std::array<std::string, 3> imageTools = { ICON_LC_CROP, ICON_LC_SQUARE_BOTTOM_DASHED_SCISSORS, ICON_LC_SQUARE_DASHED_MOUSE_POINTER };
#if defined( SDL_PLATFORM_EMSCRIPTEN )
                    std::array<std::string, 3> imageTooltips = { "Crop", "Cut", "Desktop Only" };
#else
                    std::array<std::string, 3> imageTooltips = { "Crop", "Cut", "Segment Cut" };
#endif
                    std::array<Mode, 3> imageToolModes = { Mode::Crop, Mode::Cut, Mode::SegmentCut };

                    for( size_t i = 0; i < imageTools.size(); i++ )
                    {
                        ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                        if( app->mode == imageToolModes[i] )
                        {
                            color = ImGui::ColorConvertU32ToFloat4( Spectrum::PURPLE400 );
                        }

#if defined( SDL_PLATFORM_EMSCRIPTEN )
                        if( i == 2 )
                        {
                            ImGui::BeginDisabled();
                        }
#endif

                        ImGui::PushStyleColor( ImGuiCol_Button, color );
                        if( ImGui::Button( imageTools[i].c_str(), buttonSize ) )
                        {
                            submitEvent( Events::ChangeMode, { .mode = imageToolModes[i] } );
                        }
                        if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                            ImGui::SetItemTooltip( imageTooltips[i].c_str() );

#if defined( SDL_PLATFORM_EMSCRIPTEN )
                        if( i == 2 )
                        {
                            ImGui::SetItemTooltip( imageTooltips[i].c_str() );
                            ImGui::EndDisabled();
                        }
#endif

                        ImGui::SameLine( 0.0, buttonSpacing );
                        ImGui::PopStyleColor( 1 );
                    }
                }
                else
                {
                    if( ImGui::Button( ICON_LC_GROUP, buttonSize ) )
                    {
                        submitEvent( Events::MergeAndRasterizeRequest );
                    }
                    if( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_Stationary ) )
                    {
                        const char* rasterizeToolTip = app->layers.numSelected() == 1 ? "Rasterize Layer" : "Rasterize & Merge Layers";
                        ImGui::SetItemTooltip( rasterizeToolTip );
                    }
                }


                ImGui::PopStyleColor( 1 );
                ImGui::PopStyleVar( 1 );
            }
            ImGui::End();
        }

        ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, glm::vec2( 0.5f, 0.5f ) );
        if( ImGui::BeginPopupModal( "About", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "Miskeenity Canvas %s By Sava", MC_GIT_HASH );
            ImGui::Separator();
            ImGui::Text( "Special Thanks to These Open Source Libs:" );
            ImGui::Text( "SDL" );
            ImGui::Text( "Dear ImGui" );
            ImGui::Text( "stb Headers" );
            ImGui::Text( "glm" );
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

        if( app->mode == Mode::Paint )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 180 * g_uiScale - buttonSpacing ), ImGuiCond_Appearing );
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
                    acceptEditModeChanges( app->mode );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    submitEvent( Events::ResetEditLayers );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
            }
            ImGui::End();
        }
        else if( app->mode == Mode::Text )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 435 * g_uiScale - buttonSpacing ), ImGuiCond_Appearing );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 435.0 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Text Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {
                ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );

                float width = 35 * g_uiScale;
                if( ImGui::Button( ICON_LC_ALIGN_LEFT, glm::vec2( width, 0.0 ) ) )
                {
                    g_inputTextAlignment = FontManager::Alignment::Left;
                };
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( ICON_LC_ALIGN_CENTER, glm::vec2( width, 0.0 ) ) )
                {
                    g_inputTextAlignment = FontManager::Alignment::Center;
                };
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( ICON_LC_ALIGN_RIGHT, glm::vec2( width, 0.0 ) ) )
                {
                    g_inputTextAlignment = FontManager::Alignment::Right;
                };

                if( g_setinputTextFocus )
                {
                    ImGui::SetKeyboardFocusHere( 0 );
                    g_setinputTextFocus = false;
                }

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

                const char* items[] = { "Arial", "Times New Roman", "Impact" };

                if( ImGui::BeginCombo( "##Font Combo", items[static_cast<int>( g_inputTextFont )] ) )
                {
                    for( int n = 0; n < IM_ARRAYSIZE( items ); n++ )
                    {
                        const bool is_selected = ( static_cast<int>( g_inputTextFont ) == n );
                        if( ImGui::Selectable( items[n], is_selected ) )
                        {
                            g_inputTextFont = static_cast<FontManager::Font>( n );
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if( is_selected )
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::ColorEdit3( "##Text Color", &g_inputTextColor.r );

                ImGui::SeparatorText( "Outline" );

                ImGui::SliderFloat( "##Text Outline", &g_inputTextOutline, 0.0f, 1.0f );
                ImGui::ColorEdit3( "##Text Outline Color", &g_inputTextOutlineColor.r );

                ImGui::PopItemWidth();

                ImGui::SeparatorText( "" );

                width = ( ImGui::GetContentRegionAvail().x - 8 ) / 2.0;
                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    acceptEditModeChanges( app->mode );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    submitEvent( Events::ResetEditLayers );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
            }
            ImGui::End();
        }
        else if( app->mode == Mode::Crop )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 100.0 * g_uiScale - buttonSpacing ), ImGuiCond_Appearing );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 100.0 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Crop Image", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {

                float width = ( ImGui::GetContentRegionAvail().x - 8 ) * 0.5;
                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    acceptEditModeChanges( app->mode );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    submitEvent( Events::ResetEditLayers );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
            }
            ImGui::End();
        }
        else if( app->mode == Mode::Cut )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 150 * g_uiScale - buttonSpacing ), ImGuiCond_Appearing );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 150 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Cut Image Via Painted Mask", nullptr,
                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {

                ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );

                ImGui::SliderFloat( "##Mask Paint Radius", &g_paintRadius, 0.0f, 200.0f );

                ImGui::SeparatorText( "" );

                float width = ( ImGui::GetContentRegionAvail().x - 8 ) * 0.5;
                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    if( app->layerHistory.getCheckpoint().length() < app->layers.length() )
                    {
                        acceptEditModeChanges( app->mode );
                    }

                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    submitEvent( Events::ResetEditLayers );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
            }
            ImGui::End();
        }
        else if( app->mode == Mode::SegmentCut )
        {
            ImGui::SetNextWindowPos( glm::vec2( buttonSpacing, app->height - 180.0 * g_uiScale - buttonSpacing ), ImGuiCond_Appearing );
            ImGui::SetNextWindowSize( glm::vec2( 350.0, 180.0 ) * g_uiScale, ImGuiCond_FirstUseEver );

            ImGui::Begin( "Cut Image Via Auto Segmentation", nullptr,
                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
            {

                ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );
                float width = ( ImGui::GetContentRegionAvail().x - 8 ) * 0.5;

                if( !app->mlInference->pipelineValid() )
                {
                    ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( Spectrum::RED700 ), "Model files not found or are invalid" );
                    ImGui::BeginDisabled();
                }
                else if( !app->mlInference->inferenceReady() )
                {
                    ImGui::ProgressBar( -1.0f * (float)ImGui::GetTime(), ImVec2( 0.0f, 0.0f ), "Model Loading. Please Wait..." );
                    ImGui::BeginDisabled();
                }
                else if( app->mlInference.get() && app->mlInference->pipelineValid() )
                {
                    ImGui::Text( "Click image to select segmentation regions" );
                }

                if( ImGui::Button( "Reset Points", glm::vec2( width, 0.0 ) ) )
                {
                    app->mlInference->resetPoints();
                }

                ImGui::SeparatorText( "" );

                if( ImGui::Button( "Apply", glm::vec2( width, 0.0 ) ) )
                {
                    if( app->mlInference->getPoints().size() > 0 )
                    {
                        acceptEditModeChanges( app->mode );
                    }
                    else
                    {
                        submitEvent( Events::ResetEditLayers );
                    }
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }

                if( !app->mlInference.get() || !app->mlInference->pipelineValid() || !app->mlInference->inferenceReady() )
                {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine( 0.0, 8.0 );
                if( ImGui::Button( "Cancel", glm::vec2( width, 0.0 ) ) )
                {
                    submitEvent( Events::ResetEditLayers );
                    submitEvent( Events::ChangeMode, { .mode = Mode::Cursor } );
                }
            }
            ImGui::End();
        }


        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        // draw selection box
        if( app->mode == Mode::Cursor && app->dragType == CursorDragType::Select && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
        {
            drawList->AddRect( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE500 );
            drawList->AddRectFilled( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
        }

        if( app->mode == Mode::Save )
        {
            if( app->dragType == CursorDragType::Select && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
            {
                int selectMinX = std::min( app->mouseDragStart.x, app->mouseWindowPos.x );
                int selectMinY = std::min( app->mouseDragStart.y, app->mouseWindowPos.y );
                int selectMaxX = std::max( app->mouseDragStart.x, app->mouseWindowPos.x );
                int selectMaxY = std::max( app->mouseDragStart.y, app->mouseWindowPos.y );

                selectMinX = std::clamp( selectMinX, 1, app->width - 1 );
                selectMinY = std::clamp( selectMinY, 1, app->height - 1 );
                selectMaxX = std::clamp( selectMaxX, 1, app->width - 1 );
                selectMaxY = std::clamp( selectMaxY, 1, app->height - 1 );

                std::array<ImVec2, 10> pointsClockwise = { glm::vec2( 0.0f ),
                                                           glm::vec2( app->width, 0.0f ),
                                                           glm::vec2( app->width, app->height ),
                                                           glm::vec2( 0.0f, app->height ),
                                                           glm::vec2( 0.0f ),
                                                           glm::vec2( selectMinX, selectMinY ),
                                                           glm::vec2( selectMinX, selectMaxY ),
                                                           glm::vec2( selectMaxX, selectMaxY ),
                                                           glm::vec2( selectMaxX, selectMinY ),
                                                           glm::vec2( selectMinX, selectMinY ) };

                drawList->AddConcavePolyFilled( pointsClockwise.data(), pointsClockwise.size(), Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
                drawList->AddRect( app->mouseDragStart, app->mouseWindowPos, Spectrum::PURPLE500 );
            }
            else
            {
                ImGui::SetNextWindowPos( glm::vec2( buttonSpacing * 2 ), ImGuiCond_Always );
                ImGui::SetNextWindowSize( glm::vec2( 200.0f, 80.0f ) * g_uiScale, ImGuiCond_FirstUseEver );

                ImGui::Begin( "Save Image", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs );
                {
                    ImGui::SetCursorPos( ( glm::vec2( ImGui::GetWindowSize() ) - glm::vec2( ImGui::CalcTextSize( "Select Save Area" ) ) ) * 0.5f );
                    ImGui::Text( "Select Save Area" );
                }
                ImGui::End();

                drawList->AddRectFilled( glm::vec2( 0.0f ), glm::vec2( app->width, app->height ), Spectrum::PURPLE700 & 0x00FFFFFF | 0x33000000 );
            }
        }

        if( app->mode == Mode::Crop )
        {
            int index = app->layers.getSingleSelectedImage();

            Layer layer = app->layers.getUncroppedLayer( index );

            glm::vec2 uvTop    = glm::vec2( layer.uvTop ) / float( UV_MAX_VALUE );
            glm::vec2 uvBottom = glm::vec2( layer.uvBottom ) / float( UV_MAX_VALUE );

            // see if the layer is mirrored
            glm::vec3 cross = glm::cross( glm::vec3( layer.basisA, 0.0f ), glm::vec3( layer.basisB, 0.0f ) );
            float sign      = glm::sign( cross ).z;

            glm::vec2 uncroppedCornerTL = ( layer.offset - ( layer.basisA * sign + layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 uncroppedCornerBR = ( layer.offset + ( layer.basisA * sign + layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 uncroppedCornerTR = ( layer.offset + ( layer.basisA * sign - layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 uncroppedCornerBL = ( layer.offset - ( layer.basisA * sign - layer.basisB ) * 0.5f ) * app->viewParams.scale + app->viewParams.canvasPos;

            drawList->AddImageQuad( (ImTextureID)(intptr_t)app->textureManager.get( app->layers.getTexture( index ) ).textureView.Get(), uncroppedCornerTL,
                                    uncroppedCornerTR, uncroppedCornerBR, uncroppedCornerBL, uvTop, glm::vec2( uvBottom.x, uvTop.y ), uvBottom,
                                    glm::vec2( uvTop.x, uvBottom.y ) );

            drawShadedRectangleMask(
                app->width, app->height, app->selectionAabb * app->viewParams.scale + glm::vec4( app->viewParams.canvasPos, app->viewParams.canvasPos ),
                { g_transformBox.cornerHandleTL, g_transformBox.cornerHandleBR, g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBL }, drawList );

            drawList->AddLine( g_transformBox.cornerHandleTL, g_transformBox.cornerHandleTR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBR, g_transformBox.cornerHandleBL, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBL, g_transformBox.cornerHandleTL, Spectrum::PURPLE400, ceilf( g_uiScale ) );

            ImU32 color = Spectrum::PURPLE400;

            drawList->AddCircleFilled( g_transformBox.cornerHandleBR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleBR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleBL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleBL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleTR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleTR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleTL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleTL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );
        }

        if( app->mode == Mode::Cut || app->mode == Mode::SegmentCut )
        {
            int index   = app->layers.getSingleSelectedImage();
            Layer layer = app->layers.data()[index];

            // see if the layer is mirrored
            glm::vec3 cross = glm::cross( glm::vec3( layer.basisA, 0.0f ), glm::vec3( layer.basisB, 0.0f ) );
            float sign      = glm::sign( cross ).z;

            glm::vec2 cornerTL = sign > 0.0 ? g_transformBox.cornerHandleTL : g_transformBox.cornerHandleTR;
            glm::vec2 cornerBR = sign > 0.0 ? g_transformBox.cornerHandleBR : g_transformBox.cornerHandleBL;
            glm::vec2 cornerTR = sign > 0.0 ? g_transformBox.cornerHandleTR : g_transformBox.cornerHandleTL;
            glm::vec2 cornerBL = sign > 0.0 ? g_transformBox.cornerHandleBL : g_transformBox.cornerHandleBR;

            glm::vec2 uvTop    = glm::vec2( layer.uvTop ) / float( UV_MAX_VALUE );
            glm::vec2 uvBottom = glm::vec2( layer.uvBottom ) / float( UV_MAX_VALUE );

            drawList->AddImageQuad( (ImTextureID)(intptr_t)app->textureManager.get( app->layers.getTexture( index ) ).textureView.Get(), cornerTL, cornerTR,
                                    cornerBR, cornerBL, uvTop, glm::vec2( uvBottom.x, uvTop.y ), uvBottom, glm::vec2( uvTop.x, uvBottom.y ) );

            drawShadedRectangleMask(
                app->width, app->height, app->selectionAabb * app->viewParams.scale + glm::vec4( app->viewParams.canvasPos, app->viewParams.canvasPos ),
                { g_transformBox.cornerHandleTL, g_transformBox.cornerHandleBR, g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBL }, drawList );

            if( app->editMaskTextureHandle.get() && ( app->mode == Mode::Cut || ( app->mode == Mode::SegmentCut && app->mlInference->getPoints().size() ) ) )
            {
                drawList->AddImageQuad( (ImTextureID)(intptr_t)app->textureManager.get( *app->editMaskTextureHandle.get() ).textureView.Get(), cornerTL,
                                        cornerTR, cornerBR, cornerBL, uvTop, glm::vec2( uvBottom.x, uvTop.y ), uvBottom, glm::vec2( uvTop.x, uvBottom.y ),
                                        Spectrum::ORANGE600 & 0x00FFFFFF | 0x55000000 );
            }
            else
            {
                drawList->AddQuadFilled( g_transformBox.cornerHandleTL, g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBR,
                                         g_transformBox.cornerHandleBL, Spectrum::ORANGE600 & 0x00FFFFFF | 0x55000000 );
            }

            drawList->AddLine( g_transformBox.cornerHandleTL, g_transformBox.cornerHandleTR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBR, g_transformBox.cornerHandleBL, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBL, g_transformBox.cornerHandleTL, Spectrum::PURPLE400, ceilf( g_uiScale ) );
        }

        if( app->mode == Mode::SegmentCut )
        {
            int index = app->layers.getSingleSelectedImage();


            for( const glm::vec2& point : app->mlInference->getPoints() )
            {

                Layer layer     = app->layers.data()[index];
                glm::vec2 scale = static_cast<float>( mc::UV_MAX_VALUE ) / glm::vec2( layer.uvBottom - layer.uvTop );
                layer.basisA *= scale.x;
                layer.basisB *= scale.y;

                glm::vec2 uvCenter = ( glm::vec2( layer.uvTop ) + glm::vec2( layer.uvBottom ) ) / static_cast<float>( mc::UV_MAX_VALUE ) * 0.5f;
                layer.offset -= layer.basisA * ( uvCenter.x - 0.5f ) + layer.basisB * ( uvCenter.y - 0.5f );

                glm::vec2 position = ( layer.offset + layer.basisA * ( point.x - 0.5f ) + layer.basisB * ( point.y - 0.5f ) ) * app->viewParams.scale +
                                     app->viewParams.canvasPos;

                drawList->AddCircleFilled( position, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
                drawList->AddCircleFilled( position, ( HandleHalfSize * g_uiScale - ceilf( g_uiScale ) ), Spectrum::ORANGE600 );
            }
        }

        if( app->layers.numSelected() > 0 && app->dragType == CursorDragType::Select && app->mode == Mode::Cursor )
        {
            ImU32 color = Spectrum::PURPLE400;
            drawList->AddLine( g_transformBox.rotationHandleBase, g_transformBox.rotationHandle, Spectrum::PURPLE400, ceilf( g_uiScale ) );

            drawList->AddCircleFilled( g_transformBox.rotationHandle, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::RotateHandle ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.rotationHandle, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddLine( g_transformBox.cornerHandleTL, g_transformBox.cornerHandleTR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleTR, g_transformBox.cornerHandleBR, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBR, g_transformBox.cornerHandleBL, Spectrum::PURPLE400, ceilf( g_uiScale ) );
            drawList->AddLine( g_transformBox.cornerHandleBL, g_transformBox.cornerHandleTL, Spectrum::PURPLE400, ceilf( g_uiScale ) );

            drawList->AddCircleFilled( g_transformBox.cornerHandleBR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleBR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleBL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleBL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleBL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleTR, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTR ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleTR, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );

            drawList->AddCircleFilled( g_transformBox.cornerHandleTL, HandleHalfSize * g_uiScale, Spectrum::PURPLE400 );
            color = g_mouseLocationUI == MouseLocationUI::ScaleHandleTL ? Spectrum::ORANGE600 : Spectrum::Static::BONE;
            drawList->AddCircleFilled( g_transformBox.cornerHandleTL, HandleHalfSize * g_uiScale - ceilf( g_uiScale ), color );
        }

        glm::vec2 cursorAngle = glm::vec2( 1.0, 0.0 );
        int imageSelection    = app->layers.getSingleSelectedImage();
        if( imageSelection >= 0 )
        {
            cursorAngle = glm::normalize( app->layers.data()[imageSelection].basisA );
        }
        drawCursor( app->mouseWindowPos, cursorAngle, app->viewParams.scale, app->mode, app->mouseDown );

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

    std::string getInputTextString()
    {
        return g_inputTextString;
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

    FontManager::Alignment getInputTextAlignment()
    {
        return g_inputTextAlignment;
    }

    FontManager::Font getInputTextFont()
    {
        return g_inputTextFont;
    }

    bool getSaveWithTransparency()
    {
        return g_saveWithTransparency;
    }
} // namespace mc