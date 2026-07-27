#include "Util/TextSanitize.h"
#include <cstdint>

namespace {

constexpr uint32_t Invalid = (uint32_t) -1;

// Decodes one UTF-8 scalar starting at in[i]. Returns the code point and sets
// `len` to its byte length (1-4). On any malformed/overlong/surrogate sequence
// returns Invalid and sets len=1 so the caller skips a single byte and resyncs.
uint32_t decodeUtf8(std::string_view in, size_t i, int& len){
	len = 1;
	const size_t n = in.size();
	const uint8_t b0 = (uint8_t) in[i];

	if(b0 < 0x80) return b0;

	auto cont = [&](size_t k) -> int{
		if(i + k >= n) return -1;
		const uint8_t b = (uint8_t) in[i + k];
		if((b & 0xC0) != 0x80) return -1;
		return b & 0x3F;
	};

	if((b0 & 0xE0) == 0xC0){
		const int c1 = cont(1);
		if(c1 < 0) return Invalid;
		const uint32_t cp = ((b0 & 0x1Fu) << 6) | (uint32_t) c1;
		if(cp < 0x80) return Invalid; // overlong
		len = 2;
		return cp;
	}
	if((b0 & 0xF0) == 0xE0){
		const int c1 = cont(1), c2 = cont(2);
		if(c1 < 0 || c2 < 0) return Invalid;
		const uint32_t cp = ((b0 & 0x0Fu) << 12) | ((uint32_t) c1 << 6) | (uint32_t) c2;
		if(cp < 0x800) return Invalid;                  // overlong
		if(cp >= 0xD800 && cp <= 0xDFFF) return Invalid; // surrogate
		len = 3;
		return cp;
	}
	if((b0 & 0xF8) == 0xF0){
		const int c1 = cont(1), c2 = cont(2), c3 = cont(3);
		if(c1 < 0 || c2 < 0 || c3 < 0) return Invalid;
		const uint32_t cp = ((b0 & 0x07u) << 18) | ((uint32_t) c1 << 12) | ((uint32_t) c2 << 6) | (uint32_t) c3;
		if(cp < 0x10000 || cp > 0x10FFFF) return Invalid; // overlong / out of range
		len = 4;
		return cp;
	}
	return Invalid; // invalid lead byte
}

bool inRange(uint32_t cp, uint32_t lo, uint32_t hi){ return cp >= lo && cp <= hi; }

// Whitespace (other than '\n'): collapses to a single space and is trimmed.
bool isWhitespaceCp(uint32_t cp){
	switch(cp){
		case 0x09: case 0x0B: case 0x0C: case 0x0D: // tab, VT, FF, CR
		case 0x20:                                  // space
		case 0x85:                                  // NEL
		case 0xA0:                                  // NBSP
		case 0x1680:                                // Ogham space
		case 0x2028: case 0x2029:                   // line / paragraph separator
		case 0x202F: case 0x205F:                   // narrow NBSP, medium math space
		case 0x2800:                                // braille blank
		case 0x3000:                                // ideographic space
			return true;
		default:
			return inRange(cp, 0x2000, 0x200A); // en quad ... hair space
	}
}

// Zero-width / invisible formatting code points: dropped outright (the padding
// marketers append, plus emoji-joining controls and variation selectors).
bool isInvisibleCp(uint32_t cp){
	switch(cp){
		case 0x00AD: // soft hyphen
		case 0x034F: // combining grapheme joiner
		case 0x061C: // arabic letter mark
		case 0x115F: case 0x1160: // hangul fillers
		case 0x17B4: case 0x17B5: // khmer inherent vowels
		case 0x3164: // hangul filler
		case 0xFEFF: // BOM / zero-width no-break space
		case 0xFFA0: // halfwidth hangul filler
			return true;
		default: break;
	}
	return inRange(cp, 0x180B, 0x180F)   // mongolian variation/vowel separators
		|| inRange(cp, 0x200B, 0x200F)   // ZWSP, ZWNJ, ZWJ, LRM, RLM
		|| inRange(cp, 0x202A, 0x202E)   // bidi embeddings / overrides
		|| inRange(cp, 0x2060, 0x2064)   // word joiner, invisible operators
		|| inRange(cp, 0x2066, 0x206F)   // bidi isolates, deprecated format
		|| inRange(cp, 0xFE00, 0xFE0F)   // variation selectors
		|| inRange(cp, 0x1BCA0, 0x1BCA3) // shorthand format controls
		|| inRange(cp, 0x1D173, 0x1D17A) // musical beam/slur controls
		|| inRange(cp, 0xE0000, 0xE007F) // tags
		|| inRange(cp, 0xE0100, 0xE01EF);// variation selectors supplement
}

// --- Transliteration tables --------------------------------------------------

// Latin Extended-A (U+0100-U+017F), indexed by cp-0x100.
const char* const ExtA[128] = {
	"A","a","A","a","A","a","C","c", // 0100
	"C","c","C","c","C","c","D","d", // 0108
	"D","d","E","e","E","e","E","e", // 0110
	"E","e","E","e","G","g","G","g", // 0118
	"G","g","G","g","H","h","H","h", // 0120
	"I","i","I","i","I","i","I","i", // 0128
	"I","i","IJ","ij","J","j","K","k", // 0130
	"k","L","l","L","l","L","l","L", // 0138
	"l","L","l","N","n","N","n","N", // 0140
	"n","n","NG","ng","O","o","O","o", // 0148
	"O","o","OE","oe","R","r","R","r", // 0150
	"R","r","S","s","S","s","S","s", // 0158
	"S","s","T","t","T","t","T","t", // 0160
	"U","u","U","u","U","u","U","u", // 0168
	"U","u","U","u","W","w","Y","y", // 0170
	"Y","Z","z","Z","z","Z","z","s", // 0178
};

// Latin Extended Additional (U+1E00-U+1EFF) base letters (uppercase, 0 = drop),
// indexed by cp-0x1E00. Case is taken from offset parity (even = upper).
const char VietBase[256] = {
	'A','A','B','B','B','B','B','B','C','C','D','D','D','D','D','D', // 1E00
	'D','D','D','D','E','E','E','E','E','E','E','E','E','E','F','F', // 1E10
	'G','G','H','H','H','H','H','H','H','H','H','H','I','I','I','I', // 1E20
	'K','K','K','K','K','K','L','L','L','L','L','L','L','L','M','M', // 1E30
	'M','M','M','M','N','N','N','N','N','N','N','N','O','O','O','O', // 1E40
	'O','O','O','O','P','P','P','P','R','R','R','R','R','R','R','R', // 1E50
	'S','S','S','S','S','S','S','S','S','S','T','T','T','T','T','T', // 1E60
	'T','T','U','U','U','U','U','U','U','U','U','U','V','V','V','V', // 1E70
	'W','W','W','W','W','W','W','W','W','W','X','X','X','X','Y','Y', // 1E80
	'Z','Z','Z','Z','Z','Z', 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , // 1E90
	'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A', // 1EA0
	'A','A','A','A','A','A','A','A','E','E','E','E','E','E','E','E', // 1EB0
	'E','E','E','E','E','E','E','E','I','I','I','I','O','O','O','O', // 1EC0
	'O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O', // 1ED0
	'O','O','O','O','U','U','U','U','U','U','U','U','U','U','U','U', // 1EE0
	'U','U','Y','Y','Y','Y','Y','Y','Y','Y', 0 , 0 , 0 , 0 , 0 , 0 , // 1EF0
};

// Cyrillic (U+0400-U+045F), indexed by cp-0x400. BGN/PCGN-style romanization.
const char* const Cyr[0x60] = {
	"E","Yo","Dj","Gj","Ye","Dz","I","Yi",     // 0400
	"J","Lj","Nj","C","Kj","I","U","Dz",        // 0408
	"A","B","V","G","D","E","Zh","Z",           // 0410
	"I","Y","K","L","M","N","O","P",            // 0418
	"R","S","T","U","F","Kh","Ts","Ch",         // 0420
	"Sh","Shch","","Y","","E","Yu","Ya",        // 0428
	"a","b","v","g","d","e","zh","z",           // 0430
	"i","y","k","l","m","n","o","p",            // 0438
	"r","s","t","u","f","kh","ts","ch",         // 0440
	"sh","shch","","y","","e","yu","ya",        // 0448
	"e","yo","dj","gj","ye","dz","i","yi",      // 0450
	"j","lj","nj","c","kj","i","u","dz",        // 0458
};

std::string_view latin1Supp(uint32_t cp){ // U+00C0-U+00FF letters
	switch(cp){
		case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: return "A";
		case 0xC6: return "AE";
		case 0xC7: return "C";
		case 0xC8: case 0xC9: case 0xCA: case 0xCB: return "E";
		case 0xCC: case 0xCD: case 0xCE: case 0xCF: return "I";
		case 0xD0: return "D";
		case 0xD1: return "N";
		case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8: return "O";
		case 0xD9: case 0xDA: case 0xDB: case 0xDC: return "U";
		case 0xDD: return "Y";
		case 0xDE: return "Th";
		case 0xDF: return "ss";
		case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: return "a";
		case 0xE6: return "ae";
		case 0xE7: return "c";
		case 0xE8: case 0xE9: case 0xEA: case 0xEB: return "e";
		case 0xEC: case 0xED: case 0xEE: case 0xEF: return "i";
		case 0xF0: return "d";
		case 0xF1: return "n";
		case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8: return "o";
		case 0xF9: case 0xFA: case 0xFB: case 0xFC: return "u";
		case 0xFD: case 0xFF: return "y";
		case 0xFE: return "th";
		default: return ""; // 0xD7 multiplication, 0xF7 division and other symbols
	}
}

std::string_view latinExtB(uint32_t cp){ // U+0180-U+024F, European subset
	switch(cp){
		// Croatian/Serbian digraphs
		case 0x01C4: return "DZ"; case 0x01C5: return "Dz"; case 0x01C6: return "dz";
		case 0x01C7: return "LJ"; case 0x01C8: return "Lj"; case 0x01C9: return "lj";
		case 0x01CA: return "NJ"; case 0x01CB: return "Nj"; case 0x01CC: return "nj";
		case 0x01F1: return "DZ"; case 0x01F2: return "Dz"; case 0x01F3: return "dz";
		// Caron vowels
		case 0x01CD: return "A"; case 0x01CE: return "a";
		case 0x01CF: return "I"; case 0x01D0: return "i";
		case 0x01D1: return "O"; case 0x01D2: return "o";
		case 0x01D3: return "U"; case 0x01D4: return "u";
		// u-with-diaeresis + tone (Pinyin)
		case 0x01D5: case 0x01D7: case 0x01D9: case 0x01DB: return "U";
		case 0x01D6: case 0x01D8: case 0x01DA: case 0x01DC: return "u";
		case 0x01DD: return "e";
		case 0x01DE: case 0x01E0: return "A"; case 0x01DF: case 0x01E1: return "a";
		case 0x01E2: return "AE"; case 0x01E3: return "ae";
		case 0x01E4: case 0x01E6: return "G"; case 0x01E5: case 0x01E7: return "g";
		case 0x01E8: return "K"; case 0x01E9: return "k";
		case 0x01EA: case 0x01EC: return "O"; case 0x01EB: case 0x01ED: return "o";
		case 0x01F0: return "j";
		case 0x01F4: return "G"; case 0x01F5: return "g";
		case 0x01F8: return "N"; case 0x01F9: return "n";
		case 0x01FA: return "A"; case 0x01FB: return "a";
		case 0x01FC: return "AE"; case 0x01FD: return "ae";
		case 0x01FE: return "O"; case 0x01FF: return "o";
		// Double grave / inverted breve (U+0200-U+0217)
		case 0x0200: case 0x0202: return "A"; case 0x0201: case 0x0203: return "a";
		case 0x0204: case 0x0206: return "E"; case 0x0205: case 0x0207: return "e";
		case 0x0208: case 0x020A: return "I"; case 0x0209: case 0x020B: return "i";
		case 0x020C: case 0x020E: return "O"; case 0x020D: case 0x020F: return "o";
		case 0x0210: case 0x0212: return "R"; case 0x0211: case 0x0213: return "r";
		case 0x0214: case 0x0216: return "U"; case 0x0215: case 0x0217: return "u";
		// Romanian comma-below
		case 0x0218: return "S"; case 0x0219: return "s";
		case 0x021A: return "T"; case 0x021B: return "t";
		case 0x021E: return "H"; case 0x021F: return "h";
		// Dotted / macron letters
		case 0x0226: return "A"; case 0x0227: return "a";
		case 0x0228: return "E"; case 0x0229: return "e";
		case 0x022A: case 0x022C: case 0x022E: case 0x0230: return "O";
		case 0x022B: case 0x022D: case 0x022F: case 0x0231: return "o";
		case 0x0232: return "Y"; case 0x0233: return "y";
		case 0x0237: return "j";
		default: return ""; // rare African / IPA letters intentionally dropped
	}
}

std::string_view greek(uint32_t cp){ // U+0370-U+03FF, modern romanization
	switch(cp){
		// Uppercase
		case 0x0386: case 0x0391: return "A";              // Ά Α
		case 0x0388: case 0x0395: return "E";              // Έ Ε
		case 0x0389: case 0x0397: return "I";              // Ή Η
		case 0x038A: case 0x0399: case 0x03AA: return "I"; // Ί Ι Ϊ
		case 0x038C: case 0x039F: return "O";              // Ό Ο
		case 0x038E: case 0x03A5: case 0x03AB: return "Y"; // Ύ Υ Ϋ
		case 0x038F: case 0x03A9: return "O";              // Ώ Ω
		case 0x0392: return "V";
		case 0x0393: return "G";
		case 0x0394: return "D";
		case 0x0396: return "Z";
		case 0x0398: return "Th";
		case 0x039A: return "K";
		case 0x039B: return "L";
		case 0x039C: return "M";
		case 0x039D: return "N";
		case 0x039E: return "X";
		case 0x03A0: return "P";
		case 0x03A1: return "R";
		case 0x03A3: return "S";
		case 0x03A4: return "T";
		case 0x03A6: return "F";
		case 0x03A7: return "Ch";
		case 0x03A8: return "Ps";
		// Lowercase
		case 0x0390: case 0x03AF: case 0x03B9: case 0x03CA: return "i"; // ΐ ί ι ϊ
		case 0x03AC: case 0x03B1: return "a";              // ά α
		case 0x03AD: case 0x03B5: return "e";              // έ ε
		case 0x03AE: case 0x03B7: return "i";              // ή η
		case 0x03B0: case 0x03C5: case 0x03CB: case 0x03CD: return "y"; // ΰ υ ϋ ύ
		case 0x03B2: return "v";
		case 0x03B3: return "g";
		case 0x03B4: return "d";
		case 0x03B6: return "z";
		case 0x03B8: return "th";
		case 0x03BA: return "k";
		case 0x03BB: return "l";
		case 0x03BC: return "m";
		case 0x03BD: return "n";
		case 0x03BE: return "x";
		case 0x03BF: case 0x03CC: case 0x03CE: return "o"; // ο ό ώ
		case 0x03C0: return "p";
		case 0x03C1: return "r";
		case 0x03C2: case 0x03C3: return "s";              // ς σ
		case 0x03C4: return "t";
		case 0x03C6: return "f";
		case 0x03C7: return "ch";
		case 0x03C8: return "ps";
		default: return "";
	}
}

std::string_view cyrillic(uint32_t cp){ // U+0400-U+04FF
	if(cp <= 0x045F) return Cyr[cp - 0x400];
	switch(cp){
		case 0x0462: return "E"; case 0x0463: return "e"; // yat
		case 0x0472: return "F"; case 0x0473: return "f"; // fita
		case 0x0474: return "Y"; case 0x0475: return "y"; // izhitsa
		case 0x0490: return "G"; case 0x0491: return "g"; // Ukrainian ghe-upturn
		case 0x0492: return "Gh"; case 0x0493: return "gh";
		case 0x0498: return "Z"; case 0x0499: return "z";
		case 0x049A: return "Q"; case 0x049B: return "q";
		case 0x04A2: return "Ng"; case 0x04A3: return "ng";
		case 0x04AE: case 0x04B0: return "U"; case 0x04AF: case 0x04B1: return "u";
		case 0x04BA: return "H"; case 0x04BB: return "h";
		case 0x04D8: return "E"; case 0x04D9: return "e"; // schwa
		case 0x04E8: return "O"; case 0x04E9: return "o";
		default: return "";
	}
}

// `scratch` (>= 2 bytes) backs the one computed (non-literal) result so the
// view can outlive this call without allocating.
std::string_view latinExtAdd(uint32_t cp, char* scratch){ // U+1E00-U+1EFF
	const uint32_t off = cp - 0x1E00;
	if(off >= 0x96 && off <= 0x9F){
		switch(cp){
			case 0x1E96: return "h";
			case 0x1E97: return "t";
			case 0x1E98: return "w";
			case 0x1E99: return "y";
			case 0x1E9A: return "a";
			case 0x1E9B: return "s";
			case 0x1E9E: return "SS";
			default: return "";
		}
	}
	const char base = VietBase[off];
	if(base == 0) return "";
	scratch[0] = (off & 1u) ? (char) (base - 'A' + 'a') : base;
	return { scratch, 1 };
}

// Maps a non-ASCII code point to its closest ASCII form ("" = drop). `scratch`
// (>= 2 bytes) backs any computed result; literal results ignore it.
std::string_view transliterate(uint32_t cp, char* scratch){
	if(cp <= 0xBF) return "";                 // C1 controls + Latin-1 symbols
	if(cp <= 0xFF) return latin1Supp(cp);
	if(cp <= 0x17F) return ExtA[cp - 0x100];
	if(cp <= 0x24F) return latinExtB(cp);
	if(inRange(cp, 0x0370, 0x03FF)) return greek(cp);
	if(inRange(cp, 0x0400, 0x04FF)) return cyrillic(cp);
	if(inRange(cp, 0x1E00, 0x1EFF)) return latinExtAdd(cp, scratch);

	switch(cp){ // common punctuation
		case 0x2010: case 0x2011: case 0x2012: case 0x2013:
		case 0x2014: case 0x2015: case 0x2212: return "-";
		case 0x2018: case 0x2019: case 0x201A: case 0x201B: return "'";
		case 0x201C: case 0x201D: case 0x201E: case 0x201F: return "\"";
		case 0x2022: return "*";
		case 0x2026: return "...";
		default: return "";
	}
}

} // namespace

void sanitizeToAscii(std::string_view in, std::string& out){
	enum class Pending{ None, Space, Newline };

	out.clear();              // keeps existing capacity for reuse across calls
	out.reserve(in.size());   // reserve only grows; no-op when already large enough
	Pending pending = Pending::None;

	// Emits the deferred separator before real content. Skipped when `out` is
	// still empty, which trims leading whitespace.
	auto flush = [&](){
		if(!out.empty()){
			if(pending == Pending::Newline) out.push_back('\n');
			else if(pending == Pending::Space) out.push_back(' ');
		}
		pending = Pending::None;
	};

	size_t i = 0;
	while(i < in.size()){
		int len = 1;
		const uint32_t cp = decodeUtf8(in, i, len);
		i += len;

		if(cp == Invalid) continue;             // malformed byte, skip
		if(cp == 0x0A){ pending = Pending::Newline; continue; } // newline outranks space
		if(isWhitespaceCp(cp)){
			if(pending == Pending::None) pending = Pending::Space;
			continue;
		}
		if(cp < 0x20 || cp == 0x7F) continue;   // stray control characters

		if(cp <= 0x7E){                          // printable ASCII
			flush();
			out.push_back((char) cp);
			continue;
		}

		if(isInvisibleCp(cp)) continue;          // drop, keep pending whitespace

		char scratch[2];
		const std::string_view mapped = transliterate(cp, scratch);
		if(mapped.empty()) continue;             // no ASCII equivalent, dropped
		flush();
		out.append(mapped.data(), mapped.size());
	}

	// trailing pending whitespace is intentionally discarded
}

std::string sanitizeToAscii(std::string_view in){
	std::string out;
	sanitizeToAscii(in, out);
	return out;
}
