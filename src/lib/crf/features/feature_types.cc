#include "base.h"
#include "config.h"
#include "hashtable.h"
#include "tagset.h"
#include "lbfgs.h"
#include "crf/features/type.h"
#include "crf/features/context.h"
#include "crf/features/feature.h"
#include "crf/features/attributes.h"
#include "crf/features/feature_gen.h"
#include "crf/features/feature_types.h"

namespace NLP { namespace CRF {

FeatureTypes::FeatureTypes(const TagSet &tags)
  : config::OpGroup("types", "Feature types config"), tags(tags),
    use_words(*this, "words", "use word features", new WordGen(Types::words)),
    use_pos(*this, "pos", "use pos features", new PosGen(Types::pos)),
    use_prev_words(*this, "prev_words", "use previous word features", new PrevWordGen(Types::words)),
    use_next_words(*this, "next_words", "use next word features", new NextWordGen(Types::words)),
    use_prev_pos(*this, "prev_pos", "use previous pos features", new PrevPosGen(Types::pos)),
    use_next_pos(*this, "next_pos", "use next pos features", new NextPosGen(Types::pos)) { }

void FeatureTypes::get_tagpair(Raws &raws, TagPair &tp, int i) {
  if (i == 0) {
    tp.prev = Tag(Sentinel::val);
    tp.curr = tags.canonize(raws[0]);
  }
  else {
    tp.prev = tags.canonize(raws[i-1]);
    tp.curr = tags.canonize(raws[i]);
  }
}

void FeatureTypes::generate(Attributes &attributes, Sentence &sent, Contexts &contexts, Raws &raws, const bool extract) {
  for(int i = 0; i < sent.words.size(); ++i) {
    for (Children::iterator j = _children.begin(); j != _children.end(); ++j) {
      OpType *op = reinterpret_cast<OpType *>(*j);
      if ((*op)()) {
        TagPair tp;
        get_tagpair(raws, tp, i);
        if (extract)
          op->generate(attributes, sent, tp, i);
        else {
          contexts[i].klasses = tp;
          contexts[i].index = i;
          op->generate(attributes, sent, contexts[i], i);
        }
      }
    }
  }
}

} }
