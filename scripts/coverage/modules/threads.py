#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Imports.
from Queue import Queue
from threading import Thread


class Worker(Thread):
    def __init__(self, thread_pool):
        """ Initialize a worker, making it aware of the thread pool where it is
            living.

            Arguments:
                thread_pool (ThreadPool): the thread pool in where the worker
                                          is stored.
        """

        Thread.__init__(self)
        self.thread_pool = thread_pool
        self.daemon = True
        self.start()

    def run(self):
        """ Get a task (function + arguments) to execute and assign it to a
            worker. The worker will execute it and then, the task will be
            marked as done.
        """

        while True:
            function, args = self.thread_pool.get()
            function(*args)
            self.thread_pool.task_done()


class ThreadPool:
    def __init__(self, nb_threads):
        """ Initialize the thread pool with the number of threads passed as
            argument. Then, initialize the workers.

            Arguments:
                nb_threads (int): the number of threads in the thread pool.
        """

        self.thread_pool = Queue(nb_threads)
        for _ in range(nb_threads):
            Worker(self.thread_pool)

    def addWorker(self, function, *args):
        """ Add a task into the thread pool.

            Arguments:
                function (function): the function the worker is charged to
                                     execute.
                *args (dict): the arguments of the function to execute.
        """

        self.thread_pool.put((function, args))

    def waitWorkers(self):
        """ Wait for the workers to finish their execution. This will block the
            script until they all finish.
        """

        self.thread_pool.join()
