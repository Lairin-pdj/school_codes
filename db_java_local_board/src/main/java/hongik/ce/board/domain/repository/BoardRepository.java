package hongik.ce.board.domain.repository;

import hongik.ce.board.domain.entity.BoardEntity;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;

public interface BoardRepository extends JpaRepository<BoardEntity, Long> {
    // 검색 직접 호출
    public List<BoardEntity> findByTitleContaining(String keyword);
}

