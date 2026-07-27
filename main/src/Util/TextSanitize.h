#ifndef ARTEMIS_FIRMWARE_TEXTSANITIZE_H
#define ARTEMIS_FIRMWARE_TEXTSANITIZE_H

#include <string>
#include <string_view>

/**
 * Cleans free-form UTF-8 text (e.g. ANCS notification title/body) down to
 * strictly 7-bit ASCII output, containing only printable ASCII (0x20-0x7E)
 * and the newline '\n'.
 *
 * Processing:
 *  - Drops invisible / zero-width formatting code points (the email "preheader"
 *    padding iOS forwards: soft hyphen, combining grapheme joiner, ZWSP/ZWNJ/ZWJ,
 *    BOM, bidi marks, variation selectors, ...) and emoji.
 *  - Transliterates letters to their closest ASCII form so foreign text stays
 *    readable: all Latin scripts (accents/diacritics stripped, ligatures expanded),
 *    Cyrillic romanized (BGN/PCGN-style), Greek romanized (modern scheme), plus
 *    common punctuation (dashes -> '-', curly quotes -> '/", ellipsis -> "...").
 *    Anything with no ASCII equivalent (other symbols, CJK, ...) is dropped.
 *  - Normalizes whitespace: '\n' is preserved as a line break, every other
 *    whitespace char becomes a space, runs collapse to a single separator
 *    (newline wins over space), and leading/trailing whitespace is trimmed.
 *
 * The output-parameter overload fills `out` (cleared first, capacity reused) so
 * a caller holding a persistent buffer avoids a per-call allocation; prefer it
 * on hot paths. The returning overload is a convenience wrapper for fresh
 * destinations where the move-return is already optimal.
 */
void sanitizeToAscii(std::string_view in, std::string& out);
std::string sanitizeToAscii(std::string_view in);

#endif //ARTEMIS_FIRMWARE_TEXTSANITIZE_H
