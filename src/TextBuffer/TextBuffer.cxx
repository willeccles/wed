#include "TextBuffer.hxx"

TextBuffer::TextBuffer() {
  /* Push the EOF span */
  spans.push_back({{nullptr, 0}, {nullptr, 0}});
}

void TextBuffer::NewEpoch() {
  epoch++;
  /*
  for (iterator &i : persisted_iterators) {
    i.epoch++;
  }
  */
}