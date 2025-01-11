(import (
  fetchTarball {
    url = "https://github.com/edolstra/flake-compat/archive/master.tar.gz";
    sha256 = "0m4gyjxqf9nyxhm0wygk5pdw3j5mcz7qnvyk0xkgd75qbdxmq6k5";
  }
) {
  src = ./.;
}).defaultNix 