package hongik.ce.board.service;

import hongik.ce.board.domain.entity.BoardEntity;
import hongik.ce.board.domain.repository.BoardRepository;
import hongik.ce.board.dto.BoardDto;
import lombok.AllArgsConstructor;
import org.springframework.stereotype.Service;

import javax.transaction.Transactional;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

@AllArgsConstructor
@Service
public class BoardService {
    private BoardRepository boardRepository;

    // 게시글 추가
    @Transactional
    public Long savePost(BoardDto boardDto) {
        return boardRepository.save(boardDto.toEntity()).getId();
    }

    // 게시글 목록 조회
    @Transactional
    public List<BoardDto> getBoardlist() {
        List<BoardEntity> boardEntities = boardRepository.findAll();
        List<BoardDto> boardDtoList = new ArrayList<>();

        for ( BoardEntity boardEntity : boardEntities) {
            BoardDto boardDTO = BoardDto.builder()
                    .id(boardEntity.getId())
                    .title(boardEntity.getTitle())
                    .content(boardEntity.getContent())
                    .writer(boardEntity.getWriter())
                    .createdDate(boardEntity.getCreatedDate())
                    .build();

            boardDtoList.add(boardDTO);
        }

        return boardDtoList;
    }

    // 여기서 부터 작성, 주석을 해제하고 사용하세요
    // 게시글 상세 내용 조회 처리
    @Transactional
    public BoardDto getPost(Long id) {

        // 게시글의 entity 내용을 조회할 수 있도록 작성
        BoardEntity boardEntity = boardRepository.getOne(id);

        BoardDto boardDTO = convertEntityToDto(boardEntity);

        return boardDTO;
    }

    // 게시글 삭제
    @Transactional
    public void deletePost(Long id) {

        // 게시글을 삭제할 수 있도록 작성
        BoardEntity boardEntity = boardRepository.getOne(id);
        boardRepository.delete(boardEntity);

    }

    // 게시글 검색
    @Transactional
    public List<BoardDto> searchPosts(String keyword) {
        List<BoardEntity> boardEntities = boardRepository.findByTitleContaining(keyword);
        List<BoardDto> boardDtoList = new ArrayList<>();

        // 게시글을 검색할 수 있도록 작성
        for ( BoardEntity boardEntity : boardEntities) {
            BoardDto boardDTO = convertEntityToDto(boardEntity);
            boardDtoList.add(boardDTO);
        }
        return boardDtoList;
    }

    //  Entity를 Dto로 변환하는 작업이 중복해서 발생했었는데, 이를 함수로 처리하도록 개선
    private BoardDto convertEntityToDto(BoardEntity boardEntity) {
        return BoardDto.builder()
                .id(boardEntity.getId())
                .title(boardEntity.getTitle())
                .content(boardEntity.getContent())
                .writer(boardEntity.getWriter())
                .createdDate(boardEntity.getCreatedDate())
                .build();
    }
}