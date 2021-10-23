#pragma once

class CelestiaCore;
class Selection;

namespace celestia
{
class SDL_InfoViewer;

void renderContextMenu(CelestiaCore* appCore, const Selection& selection, SDL_InfoViewer& infoViewer);
}
