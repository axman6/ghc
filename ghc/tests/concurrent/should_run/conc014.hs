import Concurrent
import Exception

-- Test blocking of async exceptions in an exception handler.
-- The exception raised in the main thread should not be delivered
-- until the first exception handler finishes.
main = do
  main_thread <- myThreadId
  m <- newEmptyMVar
  forkIO (do { takeMVar m;  raiseInThread main_thread (ErrorCall "foo") })
  (error "wibble")
	`catchAllIO` (\e -> do putMVar m ()
			       threadDelay 500000
			       putStrLn "done.")
  (threadDelay 500000)
	`catchAllIO` (\e -> putStrLn ("caught: " ++ show e))

