#pragma once

#include "../background/opBackground.h"
#include "../imageProc/ImageProc.h"
#include "OpenCvModule.h"

#include <string>
#include <vector>

namespace opcv::bridge {

MatchColorMode ParseColorMode(long color_mode);

std::wstring BuildMatchJson(const MatchResult &match, bool ok);
std::wstring BuildNamedMatchJson(const NamedMatchResult &match, bool ok);
std::wstring BuildAllMatchesJson(const std::vector<NamedMatchResult> &matches, bool ok);

bool ParseTemplateNames(const wchar_t *text, std::vector<std::wstring> &names);
bool ParseScaleList(const wchar_t *text, std::vector<double> &scales);

bool CaptureRegion(
    opBackground &background,
    ImageProc &image_proc,
    long x,
    long y,
    long width,
    long height,
    ImageHandle &source,
    Region &region,
    int &origin_x,
    int &origin_y);

} // namespace opcv::bridge
