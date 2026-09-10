_pipeline.__suffix__:
    class TranslationError(RuntimeError):
        packet_id: int
        stage: str
