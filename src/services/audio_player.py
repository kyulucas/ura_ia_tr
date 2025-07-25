import queue

class AudioPlayer:
    def __init__(self):
        self.input_queue = queue.Queue()

    def write(self, chunk):
        """Recebe um chunk de áudio do baresip e coloca na fila interna."""
        self.input_queue.put(chunk)

    def get_chunk(self, timeout=None):
        """Permite ao código principal pegar um chunk da fila."""
        return self.input_queue.get(timeout=timeout)