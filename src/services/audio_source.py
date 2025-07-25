import queue

class AudioSource:
    def __init__(self):
        self.output_queue = queue.Queue()

    def read(self, size):
        """Lê um chunk de áudio da fila de saída para enviar ao baresip."""
        try:
            chunk = self.output_queue.get(timeout=1)
            return chunk[:size]
        except queue.Empty:
            return b''  # Retorna silêncio se não houver áudio

    def put_chunk(self, chunk):
        """Permite ao código principal colocar um chunk na fila de saída."""
        self.output_queue.put(chunk) 