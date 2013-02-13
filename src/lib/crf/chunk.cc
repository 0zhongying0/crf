#include "base.h"

#include "config.h"
#include "io.h"
#include "lbfgs.h"
#include "hashtable/size.h"
#include "shared.h"
#include "lexicon.h"
#include "tagset.h"
#include "crf/nodepool.h"
#include "crf/lattice.h"
#include "crf/state.h"
#include "crf/features.h"
#include "crf/tagger.h"
#include "crf/chunk.h"

namespace NLP { namespace CRF {

const std::string Chunk::name = "chunk";
const std::string Chunk::desc = "description";
const std::string Chunk::reader = "conll";

class Chunk::Impl : public Tagger::Impl {
  protected:
    typedef Tagger::Impl Base;

    virtual void run_tag(Reader &reader, Writer &writer) {
      load();
      Sentence sent;
      State state(tags.size());

      while (reader.next(sent)) {
        tag(state, sent);
        writer.next(sent);
        sent.reset();
        state.reset();
      }
    }

    virtual void tag(State &state, Sentence &sent) {
      for (size_t i = 0; i < sent.size(); ++i) {
        registry.add_features(lexicon, sent, state.dist, i);
        state.lattice.viterbi(tags, state.dist);
      }
      state.lattice.print(std::cout, tags, sent.size());
      state.lattice.best(tags, sent.chunks, sent.size());
    }

    virtual void _pass1(Reader &reader) {
      Sentence sent;
      uint64_t max_size = 0;
      while (reader.next(sent)) {
        for (size_t i = 0; i < sent.size(); ++i) {
          lexicon.add(sent.words[i]);
          pos.add(sent.pos[i]);
          tags.add(sent.chunks[i]);
          if (sent.size() > max_size)
            max_size = sent.size();
        }
        sent.reset();
      }

      lexicon.save(preface);
      pos.save(preface);
      tags.save(preface);
      model.max_size(max_size);
    }

    virtual void _pass2(Reader &reader) {
      Sentence sent;
      Contexts contexts; //not used in this pass
      while (reader.next(sent)) {
        registry.generate(attributes, lexicon, tags, sent, sent.chunks, contexts, true);
        sent.reset();
      }

      attributes.save_attributes(cfg.attributes(), preface);
    }

    virtual void _pass3(Reader &reader, Instances &instances) {
      Sentence sent;
      while (reader.next(sent)) {
        Contexts contexts(sent.words.size());
        instances.push_back(contexts);
        registry.generate(attributes, lexicon, tags, sent, sent.chunks, instances.back(), false);
        sent.reset();
      }
    }

    virtual void reg(void) {
      Tagger::Impl::reg();

      registry.reg(Types::p, types.use_pos, new PosGen(p_dict, true, false));
      registry.reg(Types::pp, types.use_prev_pos, new OffsetPosGen(p_p_dict, -1, true, false));
      registry.reg(Types::ppp, types.use_prev_pos, new OffsetPosGen(pp_p_dict, -2, true, false));
      registry.reg(Types::np, types.use_next_pos, new OffsetPosGen(n_p_dict, 1, true, false));
      registry.reg(Types::nnp, types.use_next_pos, new OffsetPosGen(nn_p_dict, 2, true, false));

      registry.reg(Types::ppp_pp, types.use_pos_bigrams, new BigramPosGen(ppp_pp_p_dict, -2, true, false));
      registry.reg(Types::pp_p, types.use_pos_bigrams, new BigramPosGen(pp_p_p_dict, -1, true, false));
      registry.reg(Types::p_np, types.use_pos_bigrams, new BigramPosGen(p_np_p_dict, 0, true, false));
      registry.reg(Types::np_nnp, types.use_pos_bigrams, new BigramPosGen(np_nnp_p_dict, 1, true, false));

      //registry.reg(Types::pps, types.use_prev_shape, new OffsetShapeGen(a_dict, -2, true, true));
      //registry.reg(Types::ps, types.use_prev_shape, new OffsetShapeGen(a_dict, -1, true, false));
      //registry.reg(Types::s, types.use_shape, new ShapeGen(a_dict, true, false));
      //registry.reg(Types::ns, types.use_next_shape, new OffsetShapeGen(a_dict, 1, true, false));
      //registry.reg(Types::nns, types.use_next_shape, new OffsetShapeGen(a_dict, 2, true, true));
    }

    virtual void load(void) {
      pos.load();
      Tagger::Impl::load();
    }

  public:
    TagSet pos;
    TagDict p_dict;
    TagDict p_p_dict;
    TagDict pp_p_dict;
    TagDict n_p_dict;
    TagDict nn_p_dict;

    BiTagDict ppp_pp_p_dict;
    BiTagDict pp_p_p_dict;
    BiTagDict p_np_p_dict;
    BiTagDict np_nnp_p_dict;

    const bool use_pos;
    const bool use_prev_pos;
    const bool use_next_pos;

    Impl(Chunk::Config &cfg, Types &types, const std::string &preface)
      : Base(cfg, types, preface), pos(cfg.pos()), p_dict(pos), p_p_dict(pos),
        pp_p_dict(pos), n_p_dict(pos), nn_p_dict(pos), ppp_pp_p_dict(pos),
        pp_p_p_dict(pos), p_np_p_dict(pos), np_nnp_p_dict(pos),
        use_pos(types.use_pos()), use_prev_pos(types.use_prev_pos()),
        use_next_pos(types.use_next_pos()) { }

};

Chunk::Chunk(Chunk::Config &cfg, Types &types, const std::string &preface)
  : Tagger(cfg, preface, new Impl(cfg, types, preface)) { }

void Chunk::train(Reader &reader) { _impl->train(reader); }

void Chunk::run_tag(Reader &reader, Writer &writer) { _impl->run_tag(reader, writer); }

void Chunk::tag(State &state, Sentence &sent) { _impl->tag(state, sent); }

void Chunk::extract(Reader &reader, Instances &instances) {
  _impl->extract(reader, instances);
}

} }