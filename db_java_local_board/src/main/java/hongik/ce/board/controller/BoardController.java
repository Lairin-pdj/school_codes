package hongik.ce.board.controller;

import hongik.ce.board.dto.BoardDto;
import hongik.ce.board.service.BoardService;

import lombok.AllArgsConstructor;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;

import java.util.List;


@Controller
@AllArgsConstructor
public class BoardController {
    private BoardService boardService;

    // 게시글 목록 출력
    // Model 객체를 통해 View에 데이터 전달
    @GetMapping("/")
    public String list(Model model) {
        List<BoardDto> boardList = boardService.getBoardlist();

        model.addAttribute("boardList", boardList);
        return "board/list.html";
    }

    // 게시글 추가 페이지
    @GetMapping("/post")
    public String write() {
        return "board/write.html";
    }
    // 게시글 추가 후 초기 페이지로 돌아감
    @PostMapping("/post")
    public String write(BoardDto boardDto) {
        boardService.savePost(boardDto);

        return "redirect:/";
    }
    // 여기서 부터 작성, 주석을 해제하고 사용하세요
    // 게시글 상세 조회 페이지
    // detail.html을 반환해야함
    @GetMapping("/post/{no}")
    public String detail(@PathVariable("no") Long no, Model model) {
        BoardDto boardPost = boardService.getPost(no);
        model.addAttribute("boardDto", boardPost);
        return "board/detail.html";
    }

    // 게시글 수정 페이지
    // update.html을 반환해야함
    @GetMapping("/post/edit/{no}")
    public String edit(@PathVariable("no") Long no, Model model) {
        BoardDto boardPost = boardService.getPost(no);
        model.addAttribute("boardDto", boardPost);
        return "board/update.html";
    }

    // 게시글 수정
    // 수정 후 초기 페이지로 돌아가야함
    // 게시글 추가
    @PutMapping("/post/edit/{no}")
    public String update(BoardDto boardDTO) {
        boardService.savePost(boardDTO);
        return "redirect:/";
    }

    // 게시글 삭제
    // 삭제 후 초기 페이지로 돌아가야함
    @DeleteMapping("/post/{no}")
    public String delete(@PathVariable("no") Long no) {
        boardService.deletePost(no);
        return "redirect:/";
    }

    // 게시글 검색
    // 클라이언트에서 넘겨주는 keyword를 검색어로 활용함
    // 검색한 결과를 list.html를 이용해 반환함
    @GetMapping("/board/search")
    public String search(@RequestParam(value="keyword") String keyword, Model model) {
        List<BoardDto> boardList = boardService.searchPosts(keyword);

        model.addAttribute("boardList", boardList);
        return "board/list.html";
    }
}