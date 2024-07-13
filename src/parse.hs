#!/usr/bin/env runhaskell


import System.IO (isEOF)
import Data.List
import Data.Maybe

parseMotorController :: String -> Maybe String
parseMotorController str
  | isPrefixOf "1" str = let
                           rpm =  unwords $ take 2 $ words $ drop 1 str --rpm
                            -- current =  unwords $ take 1 $ words $ drop 3 str --rpm
                           current = unwords $ take 2 $ drop 3 $ words $ str
                         in return $ rpm ++ "\n" ++ current
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

