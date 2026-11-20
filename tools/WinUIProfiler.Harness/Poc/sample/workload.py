from collections.abc import Iterable


class Catalog:
    def __init__(self, labels: Iterable[str]) -> None:
        self._labels = tuple(labels)

    def find(self, query: str) -> tuple[int, ...]:
        normalized_query = query.casefold()
        return tuple(
            index
            for index, label in enumerate(self._labels)
            if label.casefold() == normalized_query
        )
