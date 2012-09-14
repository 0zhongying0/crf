#include "base.h"
#include "config.h"
#include "hashtable.h"
#include "tagset.h"
#include "crf/features/type.h"
#include "crf/features/context.h"
#include "crf/features/feature.h"
#include "crf/features/attributes.h"
#include "crf/features/feature_gen.h"
#include "crf/features/feature_types.h"

namespace NLP { namespace CRF {

const Type FeatureTypes::words = {"word features", "w", 0};

FeatureTypes::FeatureTypes(const TagSet &tags)
  : config::Config("types", "Feature types config"), tags(tags),
    use_words(*this, "words", "use word features", new WordGen(FeatureTypes::words)) { }

void FeatureTypes::generate(Attributes &attributes, Sentence &sent) {
  for(int i = 0; i <= sent.words.size(); ++i) {
    Tag prev, curr;
    if (i == sent.words.size()) {
      prev = tags.canonize(sent.entities[i-1]);
      curr = Tag((uint16_t)1);
    }
    else if (i == 0) {
      prev = Tag((uint16_t)0);
      curr = tags.canonize(sent.entities[i]);
    }
    else {
      prev = tags.canonize(sent.entities[i-1]);
      curr = tags.canonize(sent.entities[i]);
    }
    TagPair tp(prev, curr);
    use_words.generate(attributes, sent, tp, i);
  }
}

void FeatureTypes::generate(Attributes &attributes, Sentence &sent, Contexts &contexts) {
  for(size_t i = 0; i <= sent.words.size(); ++i) {
    Context context;
    contexts.push_back(context);
    use_words.generate(attributes, sent, contexts.back(), i);
  }
}

} }