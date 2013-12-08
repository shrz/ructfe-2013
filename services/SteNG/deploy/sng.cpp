#include "sng.h"

#include <sstream>
#include <iterator>
#include <iomanip>
#include <cstring>
#include <cctype>
#include <algorithm>

std::string _ (const std::string & s) {
	if (s.back () == ':' || s.back () == ';')
		return s.substr (0, s.length () - 1);

	return s;
}

template <typename T>
T get_num (std::istringstream & is) {
	long long x;
	is >> x;

	return static_cast <T> (x);
}

sng_IHDR::sng_IHDR () :
	width (0),
	height (0),
	depth (0), 
	color (false),
	palette (false),
	alpha (false) { }

sng_IHDR parse_IHDR (const std::string & s) {
	sng_IHDR r;

	std::istringstream is (s);
	std::string field;
	while (is >> field) {
		field = _ (field);

		if (field == "height")
			r.height = get_num <long> (is);
		else if (field == "width")
			r.width = get_num <long> (is);
		else if (field == "bitdepth")
			r.depth = get_num <unsigned char> (is);
		else if (field == "using") {
			std::string opt;

			while (is >> opt) {
				opt = _ (opt);

				if (opt == "color")
					r.color = true;
				else if (opt == "palette")
					r.palette = true;
				else if (opt == "alpha")
					r.alpha = true;
			}
		}
	}

	return r;
}

sng_gAMA::sng_gAMA () :
	value (1.0) { }

sng_gAMA parse_gAMA (const std::string & s) {
	sng_gAMA r;
	
	std::istringstream is (s);
	is >> r.value;

	return r;
}

sng_PLTE::sng_PLTE () :
	colors (0) { }

std::string replace (const std::string & s, const char * chars, char symbol) {
	std::string r = s;

	for (std::string::iterator it = r.begin (); it != r.end (); ++ it)
		if (strchr (chars, * it))
			* it = symbol;

	return r;
}

pcolor::pcolor () :
	r (0),
	g (0),
	b (0),
	a (0) { }

pcolor::pcolor (int r, int g, int b, int a) :
	r (r),
	g (g),
	b (b),
	a (a) { }

bool operator == (const pcolor & lhs, const pcolor & rhs) {
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

sng_PLTE parse_PLTE (const std::string & s) {
	sng_PLTE r;

	std::istringstream is (replace (s, "(),", ' '));

	int pr, pg, pb;
	while (is >> pr >> pg >> pb)
		r.colors.push_back (pcolor (pr, pg, pb, 0));

	return r;
}

sng_hIST::sng_hIST () :
	values (0) { }

sng_hIST parse_hIST (const std::string & s) {
	sng_hIST r;	

	std::istringstream is (_ (s));
	r.values.assign (std::istream_iterator <short> (is), std::istream_iterator <short> ());

	return r;
}

sng_tIME::sng_tIME () :
	year (1970),
	month (0),
	day (0),
	hour (0),
	minute (0),
	second (0) { }

sng_tIME::sng_tIME (short year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second) :
	year (year),
	month (month),
	day (day),
	hour (hour),
	minute (minute),
	second (second) { }


sng_tIME parse_tIME (const std::string & s) {
	sng_tIME r;

	std::istringstream is (s);
	std::string field;
	while (is >> field) {
		field = _ (field);

		if (field == "year")
			r.year = get_num <short> (is);
		else if (field == "month")
			r.month = get_num <unsigned char> (is);
		else if (field == "day")
			r.day = get_num <unsigned char> (is);
		else if (field == "hour")
			r.hour = get_num <unsigned char> (is);
		else if (field == "minute")
			r.minute = get_num <unsigned char> (is);
		else if (field == "second")
			r.second = get_num <unsigned char> (is);
	}

	return r;
}

sng_tEXt::sng_tEXt () :
	keyword (""),
	text ("") { }

sng_tEXt parse_tEXt (const std::string & s) {
	sng_tEXt r;

	std::istringstream is (s);
	std::string field;
	while (is >> field) {
		field = _ (field);

		if (field == "keyword")
			is >> r.keyword;
		else if (field == "text")
			std::getline (is, r.text);
	}

	return r;
}

sng_IMAGE::sng_IMAGE () :
	pixels (0) { }

int hex2byte (std::string::const_iterator & it, unsigned char depth) {
	int r = 0;
	for (int i = 0; i < (depth >> 2); ++ i) {
		r <<= 4;

		if (std::isdigit (* it)) r += * it - '0';
		else r += (* it - 'a') + 10;

		++ it;		
	}

	return r;
}

color_t hex2color (const std::string & s, unsigned char depth) {
	color_t c;

	std::string::const_iterator it = s.begin ();
	c.c.r = hex2byte (it, depth);
	c.c.g = hex2byte (it, depth);
	c.c.b = hex2byte (it, depth);
	if (it != s.end ())
		c.c.a = hex2byte (it, depth);

	return c;
}

color_t::color_t () :
	c (0, 0, 0, 0) { }

color_t::color_t (unsigned char idx) :
	idx (idx) { }

sng_IMAGE parse_IMAGE (const std::string & s, unsigned char depth, bool has_palette) {
	sng_IMAGE r;

	std::istringstream is (s);
	std::string line;
	while (std::getline (is, line)) {
		if (line.empty () || line.find ("pixels hex") != std::string::npos)
			continue;

		r.pixels.push_back (std::vector <color_t> ());
		if (has_palette) {
			std::string::const_iterator it = line.begin ();

			while (it != line.end ())
				r.pixels.back ().push_back (color_t (hex2byte (it, 8)));
		}
		else {
			std::istringstream iss (line);
			std::string c;
			while (iss >> c)
				r.pixels.back ().push_back (hex2color (c, depth));
		}
	}

	return r;
}

void parse_private (sng_private & r, const std::string & s, const std::string & chunk) {
	std::string::size_type tag_begin = chunk.find_first_not_of (" \t");
	std::string::size_type tag_end = chunk.find_last_not_of (" \t");

	std::string::size_type data_begin = s.find_first_of ("\"");
	std::string::size_type data_end = data_begin == std::string::npos ? std::string::npos : s.find_first_of ("\"", data_begin + 1);

	r.data [chunk.substr (tag_begin, tag_end - tag_begin + 1)] = s.substr (data_begin + 1, data_end - data_begin - 1);
}

sng::sng (const std::string & s) {
	std::string::size_type it = 0;

	bool has_palette = false;
	while (it != std::string::npos) {
		it = s.find_first_not_of (" \n\t", it);

		if (it != std::string::npos) {
			std::string::size_type tag_begin = it;
			std::string::size_type tag_end = s.find_first_of (" \n\t{", it);

			std::string tag = _ (s.substr (tag_begin, tag_end - tag_begin));

			std::string::size_type fields_begin = s.find_first_of ('{', tag_end);
			std::string::size_type fields_end = s.find_first_of ('}', fields_begin);

			std::string fields;
			if (fields_begin != std::string::npos && fields_end != std::string::npos)
				fields = s.substr (fields_begin + 1, fields_end - fields_begin - 1);

			it = fields_end + 1;

			if (fields.empty ())
				continue;

			if (tag == "IHDR")
				m_IHDR = parse_IHDR (fields);
			else if (tag == "gAMA")
				m_gAMA = parse_gAMA (fields);
			else if (tag == "PLTE") {
				m_PLTE = parse_PLTE (fields);
				has_palette = true;
			}
			else if (tag == "hIST")
				m_hIST = parse_hIST (fields);
			else if (tag == "tIME")
				m_tIME = parse_tIME (fields);
			else if (tag == "tEXt")
				m_tEXt = parse_tEXt (fields);
			else if (tag == "IMAGE")
				m_IMAGE = parse_IMAGE (fields, m_IHDR.depth, has_palette);
			else if (tag == "private")
				parse_private (m_private, fields, s.substr (tag_end, fields_begin - tag_end));
		}
	}	
}

long sng::width () const {
	return m_IHDR.width;
}

long sng::height () const {
	return m_IHDR.height;
}

pcolor sng::pixel (unsigned int x, unsigned int y) const {
	if (x >= width () || y >= height ())
		return pcolor ();

	color_t r = m_IMAGE.pixels [y] [x];
	return m_IHDR.palette ? m_PLTE.colors [r.idx] : r.c;
}

void sng::pixel (unsigned int x, unsigned int y, pcolor c) {
	if (x >= width () || y >= height ())
		return;

	if (m_IHDR.palette) {
		std::vector <pcolor>::const_iterator it = std::find (m_PLTE.colors.begin (), m_PLTE.colors.end (), c);

		if (it == m_PLTE.colors.end ()) {
			if (m_PLTE.colors.size () == 255)
				return;

			m_IMAGE.pixels [y] [x].idx = m_PLTE.colors.size ();
			m_PLTE.colors.push_back (c);
		}
		else
			m_IMAGE.pixels [y] [x].idx = std::distance (m_PLTE.colors.cbegin (), it);
	}
	else
		m_IMAGE.pixels [y] [x].c = c;
}

std::string sng::keyword () const {
	return m_tEXt.keyword;
}

void sng::keyword (const std::string & s) {
	m_tEXt.keyword = s;
}

std::string sng::text () const {
	return m_tEXt.text;
}

void sng::text (const std::string & s) {
	m_tEXt.text = s;
}

std::string sng::time () const {
	std::ostringstream os;
	os << (1 + m_tIME.month) << "/" << (1 + m_tIME.day) << "/" << m_tIME.year << " " <<
		(1 + m_tIME.hour) << ":" << (1 + m_tIME.minute) << ":" << (1 + m_tIME.second);

	return os.str ();
}

void sng::time (short y, unsigned char m, unsigned char d, unsigned char hh, unsigned char mm, unsigned char ss) {
	m_tIME = sng_tIME (y, m, d, hh, mm, ss);
}

std::string sng::private_ (const std::string & chunk) const {
	if (! m_private.data.count (chunk))
		return "";

	return m_private.data.at (chunk);
}

void sng::private_ (const std::string & chunk, const std::string & text) {
	if (chunk.empty ()) {
		m_private.data.erase (chunk);
		return;
	}

	m_private.data [chunk] = text;
}

std::ostream & operator << (std::ostream & os, const sng & p) {
	os << "IHDR {" << std::endl <<
		"\twidth: " << p.m_IHDR.width << std::endl <<
		"\theight: " << p.m_IHDR.height << std::endl <<
		"\tbitdepth: " << static_cast <int> (p.m_IHDR.depth) << std::endl;
	
	if (p.m_IHDR.alpha || p.m_IHDR.palette || p.m_IHDR.color) {
		os << "\tusing ";

		if (p.m_IHDR.alpha) os << "alpha ";
		if (p.m_IHDR.color) os << "color ";
		if (p.m_IHDR.palette) os << "palette ";
		
		os << std::endl;
	}

	os << "}" << std::endl << "gAMA { " << std::fixed << std::setprecision (2) << p.m_gAMA.value << " }" << std::endl <<
		"tIME {" << std::endl <<
		"\tyear " << p.m_tIME.year << std::endl <<
		"\tmonth " << static_cast <int> (p.m_tIME.month) << std::endl <<
		"\tday " << static_cast <int> (p.m_tIME.day) << std::endl <<
		"\thour " << static_cast <int> (p.m_tIME.hour) << std::endl <<
		"\tminute " << static_cast <int> (p.m_tIME.minute) << std::endl <<
		"\tsecond " << static_cast <int> (p.m_tIME.second) << std::endl <<
		"}" << std::endl;

	if (! p.m_hIST.values.empty ()) {
		os << "PLTE {" << std::endl;
		for (std::vector <pcolor>::const_iterator it = p.m_PLTE.colors.begin (); it != p.m_PLTE.colors.end (); ++ it)
			os << "\t(" << std::setw (3) << it->r << "," << std::setw (3) << it->g << "," << std::setw (3) << it->b << ")" << std::endl;
		os << "}" << std::endl << "hIST {" << std::endl << "\t";
		std::copy (p.m_hIST.values.begin (), p.m_hIST.values.end (), std::ostream_iterator <short> (os, " "));
		os << std::endl << "}" << std::endl;
	}

	if (! p.m_tEXt.keyword.empty ())
		os << "tEXt {" << std::endl <<
		"\tkeyword: " << p.m_tEXt.keyword << std::endl <<
		"\ttext: " << p.m_tEXt.text << std::endl <<
		"}" << std::endl;

	os << "IMAGE {" << std::endl << "\tpixels hex" << std::endl;
	int w = p.m_IHDR.depth >> 2;
	if (p.m_hIST.values.empty ()) {
		for (std::vector <std::vector <color_t>>::const_iterator it = p.m_IMAGE.pixels.begin (); it != p.m_IMAGE.pixels.end (); ++ it) {
			for (std::vector <color_t>::const_iterator jt = it->begin (); jt != it->end (); ++ jt) {
				os << std::setfill ('0') << std::setw (w) << std::hex << jt->c.r <<
				      std::setfill ('0') << std::setw (w) << jt->c.g <<
				      std::setfill ('0') << std::setw (w) << jt->c.b;

				if (p.m_IHDR.alpha)
					os << std::setfill ('0') << std::setw (w) << jt->c.a;

				os << " ";
			}

			os << std::endl;
		}
	}
	else {
		for (std::vector <std::vector <color_t>>::const_iterator it = p.m_IMAGE.pixels.begin (); it != p.m_IMAGE.pixels.end (); ++ it) {
			for (std::vector <color_t>::const_iterator jt = it->begin (); jt != it->end (); ++ jt)
				os << std::setfill ('0') << std::setw (2) << std::hex << static_cast <int> (jt->idx);

			os << std::endl;
		}
	}
	os << "}" << std::endl;

	if (! p.m_private.data.empty ()) {
		for (std::map <std::string, std::string>::const_iterator it = p.m_private.data.begin (); it != p.m_private.data.end (); ++ it)
			os << "private " << it->first << " {" << std::endl <<
				"\t\"" << it->second << "\"" << std::endl <<
				"}" << std::endl;
	}

	return os;
}

