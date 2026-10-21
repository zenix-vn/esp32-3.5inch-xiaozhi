#pragma once
// UI language. English strings are the keys; tr() returns the Vietnamese text when language is Vietnamese
// and passes unknown strings (SSIDs, server messages, ...) through unchanged.
#include <Arduino.h>

enum Lang : uint8_t { LANG_VI = 0, LANG_EN = 1 };

extern uint8_t gLang;
const char *tr(const char *en);
void i18nLoad();            // reads the saved language (default Vietnamese)
void i18nSave(uint8_t lang);
bool isVi();

// Inline Vietnamese / English text for strings that are not worth a translation-table entry.
#define DL(vi, en) (isVi() ? (vi) : (en))
