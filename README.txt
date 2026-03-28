# SSH Tunnelling Projesi

## 1. Derleme
Proje klasöründe aşağıdaki komutu çalıştırın:
$ make

## 2. Çalıştırma
Sırasıyla 3 ayrı terminalde çalıştırın:

1. Sunucu:  $ ./server
2. Tünel:   $ ./tunnel 12344 127.0.0.1 12345
3. İstemci: $ ./client 127.0.0.1 12344 <isim>

## 3. Komutlar
İstemci terminalinde:
- msg <mesaj>   : Şifreli mesaj gönderir.
- file <dosya>  : Dosya transferi yapar.
- baglantikapa  : Çıkış yapar.