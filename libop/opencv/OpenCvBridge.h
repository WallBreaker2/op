#pragma once

#include "../binding/BindingSession.h"
#include "../image/ImageSearchService.h"
#include "TemplateMatcher.h"

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
    op::binding::BindingSession &binding_session,
    op::image::ImageSearchService &image_proc,
    long x,
    long y,
    long width,
    long height,
    ImageHandle &source,
    Region &region,
    int &origin_x,
    int &origin_y);

} // namespace opcv::bridge
