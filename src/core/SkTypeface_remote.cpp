/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkTypeface_remote.h"
#include "SkPaint.h"
#include "SkRemoteGlyphCache.h"
#include "SkStrikeCache.h"
#include "SkTraceEvent.h"

SkScalerContextProxy::SkScalerContextProxy(sk_sp<SkTypeface> tf,
                                           const SkScalerContextEffects& effects,
                                           const SkDescriptor* desc,
                                           sk_sp<SkStrikeClient::DiscardableHandleManager> manager)
        : SkScalerContext{std::move(tf), effects, desc}
        , fDiscardableManager{std::move(manager)} {}

unsigned SkScalerContextProxy::generateGlyphCount()  {
    SK_ABORT("Should never be called.");
    return 0;
}

uint16_t SkScalerContextProxy::generateCharToGlyph(SkUnichar) {
    SK_ABORT("Should never be called.");
    return 0;
}

void  SkScalerContextProxy::generateAdvance(SkGlyph* glyph) {
    this->generateMetrics(glyph);
}

void SkScalerContextProxy::generateMetrics(SkGlyph* glyph) {
    TRACE_EVENT1("skia", "generateMetrics", "rec", TRACE_STR_COPY(this->getRec().dump().c_str()));
    if (this->getProxyTypeface()->isLogging()) {
        SkDebugf("GlyphCacheMiss generateMetrics: %s\n", this->getRec().dump().c_str());
    }

    // Since the scaler context is being called, we don't have the needed data. Go look through
    // looking for a suitable substitute before failing.
    auto desc = SkScalerContext::DescriptorGivenRecAndEffects(this->getRec(), this->getEffects());
    SkStrikeCache::DesperationSearchForImage(*desc, glyph, &fAlloc);

    if (glyph->fMaskFormat == MASK_FORMAT_UNKNOWN) {
        glyph->zeroMetrics();
    }
    fDiscardableManager->notifyCacheMiss(SkStrikeClient::CacheMissType::kGlyphMetrics);
}

void SkScalerContextProxy::generateImage(const SkGlyph& glyph) {
    TRACE_EVENT1("skia", "generateImage", "rec", TRACE_STR_COPY(this->getRec().dump().c_str()));
    if (this->getProxyTypeface()->isLogging()) {
        SkDebugf("GlyphCacheMiss generateImage: %s\n", this->getRec().dump().c_str());
    }

    // There is no desperation search here, because if there was an image to be found it was
    // copied over with the metrics search.
    fDiscardableManager->notifyCacheMiss(SkStrikeClient::CacheMissType::kGlyphImage);
}

bool SkScalerContextProxy::generatePath(SkGlyphID glyphID, SkPath* path) {
    TRACE_EVENT1("skia", "generatePath", "rec", TRACE_STR_COPY(this->getRec().dump().c_str()));
    if (this->getProxyTypeface()->isLogging()) {
        SkDebugf("GlyphCacheMiss generatePath: %s\n", this->getRec().dump().c_str());
    }
    // Since the scaler context is being called, we don't have the needed data. Go look through
    // looking for a suitable substitute before failing.
    auto desc = SkScalerContext::DescriptorGivenRecAndEffects(this->getRec(), this->getEffects());
    bool foundPath = SkStrikeCache::DesperationSearchForPath(*desc, glyphID, path);

    fDiscardableManager->notifyCacheMiss(SkStrikeClient::CacheMissType::kGlyphPath);
    return foundPath;
}

void SkScalerContextProxy::generateFontMetrics(SkPaint::FontMetrics* metrics) {
    TRACE_EVENT1(
            "skia", "generateFontMetrics", "rec", TRACE_STR_COPY(this->getRec().dump().c_str()));
    if (this->getProxyTypeface()->isLogging()) {
        SkDebugf("GlyphCacheMiss generateFontMetrics: %s\n", this->getRec().dump().c_str());
        SkDEBUGCODE(SkStrikeCache::Dump());
    }

    // Font metrics aren't really used for render, so just zero out the data and return.
    fDiscardableManager->notifyCacheMiss(SkStrikeClient::CacheMissType::kFontMetrics);
    sk_bzero(metrics, sizeof(*metrics));
}

SkTypefaceProxy* SkScalerContextProxy::getProxyTypeface() const {
    return (SkTypefaceProxy*)this->getTypeface();
}
