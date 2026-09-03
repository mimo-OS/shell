#ifndef UTILS_H
#define UTILS_H

class QString;

/**
 * Swap mnemonic char: Qt uses '&', while dbusmenu uses '_'
 */
QString swapMnemonicChar(const QString &in, const char src, const char dst);

#endif /* UTILS_P_H */
