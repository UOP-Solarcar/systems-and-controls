#!/usr/bin/env runhaskell

import Data.List
import Data.Maybe
import System.IO (isEOF)

parseMotorController :: String -> String
parseMotorController str
  | isPrefixOf "1" str =
      let
        rpm = unwords $ take 2 $ words $ drop 1 str
        current = unwords $ take 2 $ drop 3 $ words $ str
        duty = unwords $ take 3 $ drop 5 $ words $ str
       in
        rpm ++ "\n" ++ current ++ "\n" ++ duty
  | isPrefixOf "2" str =
      let
        ahin = unwords $ take 3 $ words $ drop 1 str
        ahout = unwords $ take 3 $ drop 4 $ words $ str
       in
        ahin ++ "\n" ++ ahout
  | isPrefixOf "3" str =
      let
        whused = unwords $ take 3 $ words $ drop 1 str
        whcharged = unwords $ take 3 $ drop 4 $ words $ str
       in
        whused ++ "\n" ++ whcharged
  | isPrefixOf "4" str = unwords $ take 3 $ drop 4 $ words str -- motor temp?
  | isPrefixOf "5" str =
      let
        tacho = unwords $ take 2 $ words $ drop 1 str
        voltin = unwords $ take 3 $ drop 3 $ words $ str
        reserved = unwords $ take 2 $ drop 6 $ words $ str
       in
        voltin ++ "\n" ++ tacho ++ "\n" ++ reserved
  | otherwise = ""

parseBMS :: String -> String
parseBMS line
  | isPrefixOf "Pack Current" line =
      let
        packCurr = unwords $ take 3 $ words $ line
        packInstVolt = unwords $ take 4 $ drop 3 $ words $ line
        packsoc = unwords $ take 3 $ drop 7 $ words $ line
        relayState = unwords $ take 3 $ drop 10 $ words $ line
        checksum = unwords $ take 2 $ drop 13 $ words $ line
       in
        packCurr ++ "\n" ++ packInstVolt ++ "\n" ++ packsoc ++ "\n" ++ relayState ++ "\n" ++ checksum
  | isPrefixOf "Pack Health" line =
      let
        atc = unwords $ take 4 $ drop 3 $ words $ line
        inputvoltage = unwords $ take 4 $ drop 7 $ words $ line
       in
        atc ++ "\n" ++ inputvoltage
  | isPrefixOf "High Temperature:" line = unwords $ take 3 $ drop 13 $ words $ line
  | otherwise = ""

parse :: String -> String
parse line
  | isPrefixOf motor line = fromMaybe "" $ parseMotorController <$> (stripPrefix motor line)
  | otherwise = parseBMS line
 where
  motor = "Motor Controller ID: 0x21 Status #"

main :: IO ()
main = do
  done <- isEOF
  if done
    then return ()
    else do
      line <- getLine
      let parsed = parse line
      putStrLn parsed
      main
