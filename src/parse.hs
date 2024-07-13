#!/usr/bin/env runhaskell


import System.IO (isEOF)
import Data.List
import Data.Maybe

parseMotorController :: String -> Maybe String
parseMotorController str
  | isPrefixOf "1" str = let
                           rpm =  unwords $ take 2 $ words $ drop 1 str --rpm
                           current = unwords $ take 2 $ drop 3 $ words $ str
                           duty = unwords $ take 3 $ drop 5 $ words $ str
                         in
                           return $ rpm ++ "\n" ++ current ++ "\n" ++ duty

  | isPrefixOf "2" str = let
                           ahin =  unwords $ take 3 $ words $ drop 1 str --rpm
                           ahout = unwords $ take 3 $ drop 4 $ words $ str
                         in
                           return $ ahin ++ "\n" ++ ahout

  | isPrefixOf "3" str = let
                           whused =  unwords $ take 3 $ words $ drop 1 str --rpm
                           whcharged = unwords $ take 3 $ drop 4 $ words $ str
                         in
                           return $ whused ++ "\n" ++ whcharged
  | isPrefixOf "5" str = let
                           tacho =  unwords $ take 2 $ words $ drop 1 str --rpm
                           voltin = unwords $ take 3 $ drop 3 $ words $ str
                           reserved = unwords $ take 2 $ drop 6 $ words $ str
                         in
                           return $ voltin ++ "\n" ++ tacho ++ "\n" ++ reserved
  | otherwise = Nothing


-- parseBMS :: String -> String


parse :: String -> String
parse line
  | isPrefixOf motor line = fromMaybe " " $ (stripPrefix motor line) >>= parseMotorController
  | otherwise = " "
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

