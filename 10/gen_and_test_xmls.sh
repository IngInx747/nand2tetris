#!/usr/bin

./compiler ./ArrayTest/Main.jack > ./xmls/ArrayTest/Main.xml
./compiler ./ExpressionLessSquare/Main.jack > ./xmls/Main.xml
./compiler ./ExpressionLessSquare/Square.jack > ./xmls/Square.xml
./compiler ./ExpressionLessSquare/SquareGame.jack > ./xmls/SquareGame.xml
./compiler ./Square/Main.jack > ./xmls/Square/Main.xml
./compiler ./Square/Square.jack > ./xmls/Square/Square.xml
./compiler ./Square/SquareGame.jack > ./xmls/Square/SquareGame.xml

sh ../../tools/TextComparer.sh ./ArrayTest/Main.xml ./xmls/ArrayTest/Main.xml
sh ../../tools/TextComparer.sh ./ExpressionLessSquare/Main.xml ./xmls/ExpressionLessSquare/Main.xml
sh ../../tools/TextComparer.sh ./ExpressionLessSquare/Square.xml ./xmls/ExpressionLessSquare/Square.xml
sh ../../tools/TextComparer.sh ./ExpressionLessSquare/SquareGame.xml ./xmls/ExpressionLessSquare/SquareGame.xml
sh ../../tools/TextComparer.sh ./Square/Main.xml ./xmls/Square/Main.xml
sh ../../tools/TextComparer.sh ./Square/Square.xml ./xmls/Square/Square.xml
sh ../../tools/TextComparer.sh ./Square/SquareGame.xml ./xmls/Square/SquareGame.xml 
sh ../../tools/TextComparer.sh ./Square/SquareGame.xml ./xmls/Square/SquareGame.xml
