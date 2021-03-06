#include "base.h"

#include "hashtable.h"
#include "lexicon.h"

namespace NLP {
  typedef HT::StringEntry<uint64_t> Entry;
  typedef HT::OrderedHashTable<Entry, std::string> ImplBase;
  class Lexicon::Impl : public ImplBase, public Util::Shared {
    public:
      std::string preface;
      std::string filename;

      Impl(const size_t nbuckets, const size_t pool_size)
        : ImplBase(nbuckets, pool_size), Shared(), preface(), filename() { }
      Impl(const std::string &filename, const size_t nbuckets,
          const size_t pool_size) : ImplBase(nbuckets, pool_size), Shared(),
          preface(), filename(filename) { }

      Impl(const std::string &filename, std::istream &input,
          const size_t nbuckets, const size_t pool_size) :
        ImplBase(nbuckets, pool_size), Shared(), preface(), filename(filename) {
          load(filename, input);
        }

      using ImplBase::add;
      using ImplBase::insert;

      void add(const std::string &raw, const uint64_t freq) {
        Base::add(raw)->value += freq;
      }

      void insert(const std::string &raw, const uint64_t freq) {
        Base::add(raw)->value = freq;
      }

      void load(void) {
        std::ifstream input(filename.c_str());
        if (!input)
          throw IOException("Unable to open lexicon file", filename);
        load(filename, input);
      }

      void load(const std::string &filename, std::istream &input) {
        uint64_t nlines;

        read_preface(filename, input, preface, nlines);

        std::string word;
        uint64_t freq;
        while (input >> word >> freq) {
          ++nlines;
          if (input.get() != '\n')
            throw IOException("expected newline after frequency in lexicon file", filename, nlines);
          insert(word, freq);
        }

        if (!input.eof())
          throw IOException("could not parse word or frequency information for lexicon", filename, nlines);
      }

      void save(std::ostream &out, const std::string &preface) {
        sort_by_rev_value();
        out << preface << '\n';
        ImplBase::save(out);
      }

      const Word canonize(const std::string &raw) const {
        return canonize(raw.c_str());
      }

      const Word canonize(const char *raw) const {
        if (raw[0] == '_' && raw[1] == '_') {
          if (raw == None::str)
            return NONE;
          else if (raw == Sentinel::str)
            return SENTINEL;
        }
        return Word(reinterpret_cast<uint64_t>(find(raw)));
      }

      void canonize(const Raws &raws, Words &words) const {
        words.resize(0);
        words.reserve(raws.size());
        for (Raws::const_iterator i = raws.begin(); i != raws.end(); ++i)
          words.push_back(canonize(*i));
      }

      const char *str(const Word &word) const {
        if (word.id() == None::val)
          return None::str.c_str();
        else if (word.id() == Sentinel::val)
          return Sentinel::str.c_str();
        return reinterpret_cast<Entry *>(word.id())->str;
      }

      void str(const Words &words, Raws &raws) const {
        raws.resize(0);
        raws.reserve(words.size());
        for (Words::const_iterator i = words.begin(); i != words.end(); ++i)
          raws.push_back(str(*i));
      }

      uint64_t freq(const std::string &str) const {
        Entry *e = find(str.c_str());
        if (e)
          return e->value;
        return 0;
      }

      size_t size(void) const { return Base::_size; }
  };

  Lexicon::Lexicon(const size_t nbuckets, const size_t pool_size) :
    _impl(new Impl(nbuckets, pool_size)) { }

  Lexicon::Lexicon(const std::string &filename, const size_t nbuckets,
      const size_t pool_size) : _impl(new Impl(filename, nbuckets, pool_size)) { }

  Lexicon::Lexicon(const std::string &filename, std::istream &input,
      const size_t nbuckets, const size_t pool_size)
    : _impl(new Impl(filename, input, nbuckets, pool_size)) { }

  Lexicon::Lexicon(const Lexicon &other) : _impl(share(other._impl)) { }

  Lexicon::~Lexicon(void) { release(_impl); }

  void Lexicon::add(const std::string &raw, const uint64_t freq) { _impl->add(raw, freq); }
  void Lexicon::insert(const std::string &raw, const uint64_t freq) { _impl->insert(raw, freq); }

  void Lexicon::load(void) { _impl->load(); }
  void Lexicon::load(const std::string &filename, std::istream &input) { _impl->load(filename, input); }

  void Lexicon::save(const std::string &preface) {
    std::ofstream out(_impl->filename.c_str());
    if (!out)
      throw IOException("unable to open file for writing", _impl->filename);
    _impl->save(out, preface);
  }
  void Lexicon::save(const std::string &filename, const std::string &preface) {
    std::ofstream out(filename.c_str());
    if (!out)
      throw IOException("unable to open file for writing", filename);
    _impl->save(out, preface);
  }
  void Lexicon::save(std::ostream &out, const std::string &preface) { _impl->save(out, preface); }

  const Word Lexicon::canonize(const std::string &raw) const { return _impl->canonize(raw.c_str()); }
  const Word Lexicon::canonize(const char *raw) const { return _impl->canonize(raw); }
  void Lexicon::canonize(const Raws &raws, Words &words) const { _impl->canonize(raws, words); }

  const char *Lexicon::str(const Word &word) const { return _impl->str(word); }
  void Lexicon::str(const Words &words, Raws &raws) const { _impl->str(words, raws); }

  uint64_t Lexicon::freq(const std::string &raw) const { return _impl->freq(raw); }

  void Lexicon::sort_by_freq(void) { _impl->sort_by_rev_value(); }

  size_t Lexicon::size(void) const { return _impl->size(); }

  void Lexicon::clear(void) { _impl->clear(); }
}
