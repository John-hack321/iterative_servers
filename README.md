chat-app-a3/
├── README.md
├── iterative-connectionless/        ← Variation 1: UDP
│   ├── makefile
│   ├── data/
│   │   ├── users.txt
│   │   ├── messages.txt
│   │   └── chat_log.txt
│   ├── common/
│   │   ├── protocol.h               # shared constants, port, buffer size
│   │   ├── auth.c / auth.h          # djb2 password hashing
│   │   ├── user_manager.c / .h      # register, login, logout, search, list
│   │   └── message_handler.c / .h   # store, inbox, recent history
│   ├── server/
│   │   └── server.c                 # UDP iterative server
│   └── client/
│       └── client.c                 # UDP client (server IP from CLI arg)
│
└── iterative-connection-oriented/   ← Variation 2: TCP iterative
    ├── makefile
    ├── data/
    │   ├── users.txt
    │   ├── messages.txt
    │   └── chat_log.txt
    ├── common/
    │   ├── protocol.h
    │   ├── auth.c / auth.h
    │   ├── user_manager.c / .h
    │   ├── message_handler.c / .h
    │   └── utils.c / utils.h        # TCP length-prefixed framing
    ├── server/
    │   └── server.c                 # TCP iterative server
    └── client/
        └── client.c                 # TCP client (server IP from CLI arg)