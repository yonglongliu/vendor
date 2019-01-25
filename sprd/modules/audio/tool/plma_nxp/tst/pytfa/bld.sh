echo "TFAROOT:$TFAROOT"
CFLAGS="-I$TFAROOT -I$TFAROOT/tfa/inc -I$TFAROOT/utl/inc -I$TFAROOT/app/climax/inc"
LDFLAGS="-L$TFAROOT"
CFLAGS=$CFLAGS  LDFLAGS=$LDFLAGS python setup.py build_ext -i
